#include "Adafruit_NeoPixel.h"
#include "Adafruit_NeoTrellis.h"
#include "BetterButton.h"

Adafruit_NeoTrellis sequencerKeypad;
BetterButton* keys[25];
int modeSwitchPin = 26;
int replaceRemoveStepSwitchPin = 27;
int sequenceDirectionSwitchPin = 28;
BetterButton* keyboardOctaveUpButton = new BetterButton(29, 1);
BetterButton* keyboardOctaveDownButton = new BetterButton(30, -1);
int keyboardOctaveModifier = 0;
int keyboardOctaveModifierTempArray[25];
unsigned long lastStepTime = 0;
int stepLength;
int currentStep = 0;
int totalSteps = 16;
int channelDisplayed = -1;
int velocityAtSteps[25][16];
int keyboardOctaveModifierAtSteps[25][16];

TrellisCallback editStep(keyEvent evt) {
  if(channelDisplayed >= 0) {
    if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
      if(digitalRead(replaceRemoveStepSwitchPin) == HIGH) {
        int stepVelocity = getVelocity();
        velocityAtSteps[channelDisplayed][evt.bit.NUM] = stepVelocity;
        keyboardOctaveModifierAtSteps[channelDisplayed][evt.bit.NUM] = keyboardOctaveModifier;
        sequencerKeypad.pixels.setPixelColor(evt.bit.NUM, Adafruit_NeoPixel::ColorHSV(map(stepVelocity, 0, 127, 0, 54592), 255, 255));
      }
      else {
        velocityAtSteps[channelDisplayed][evt.bit.NUM] = -1;
        keyboardOctaveModifierAtSteps[channelDisplayed][evt.bit.NUM] = -5;
        sequencerKeypad.pixels.setPixelColor(evt.bit.NUM, 255, 255, 255);
      }     
      sequencerKeypad.pixels.show();
    }
    else if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
      checkPixel(evt.bit.NUM);
    }
  }
  return 0;
}

void setup() {
  Serial.begin(9600);
  sequencerKeypad.begin();
  for(int i = 0; i < 25; i++) {
    keys[i] = new BetterButton(i, i + 48);
    keys[i]->pressHandler(onPress);
    keys[i]->releaseHandler(onRelease);
    keyboardOctaveModifierTempArray[i] = -5;
    for(int j = 0; j < 16; j++) {
      velocityAtSteps[i][j] = -1;
      keyboardOctaveModifierAtSteps[i][j] = -5;
      if(i == 0) {
        sequencerKeypad.activateKey(j, SEESAW_KEYPAD_EDGE_RISING);
        sequencerKeypad.registerCallback(j, editStep);
      }
    }   
  }
  pinMode(modeSwitchPin, INPUT);
  pinMode(replaceRemoveStepSwitchPin, INPUT);
  pinMode(sequenceDirectionSwitchPin, INPUT);
}

void loop() {
  for(int i = 0; i < 25; i++) {
    keys[i]->process();
  }
  if(digitalRead(modeSwitchPin) == LOW) {

    // if this block of code works, delete checkPixels(). otherwise, replace it with checkPixels().
    for(int i = 0; i < 15; i++) {
      checkPixel(i);
    }

    sequence();
  } 
}

void sequence() {
  stepLength = analogRead(A15);
  if(millis() >= lastStepTime + stepLength) {
    lastStepTime = millis();
    if(digitalRead(sequenceDirectionSwitchPin) == HIGH) {
      previousStep();
    }
    else {   
      nextStep();
    } 
    for(int i = 0; i < 120; i++) {
      usbMIDI.sendNoteOff(i, 0, 1);
    }    
    for(int i = 0; i < 25; i++) {
      if(velocityAtSteps[i][currentStep] >= 0 && keyboardOctaveModifierAtSteps[i][currentStep] >= -4) {
        usbMIDI.sendNoteOn((i + 48) + (12 * keyboardOctaveModifierAtSteps[i][currentStep]), velocityAtSteps[i][currentStep], 1);    
      }
    }    
  }
}

void onPress(int val) {
  if(val >= 60) {
    if(digitalRead(modeSwitchPin) == HIGH) {
      usbMIDI.sendNoteOn(val + (12 * keyboardOctaveModifier), getVelocity(), 1);
      keyboardOctaveModifierTempArray[val - 48] = keyboardOctaveModifier;
    }
    else {
      channelDisplayed = val - 48;
      Serial.println(channelDisplayed);
    }
  }
  else {
    if(keyboardOctaveModifier < 4 && keyboardOctaveModifier > -4) {
      keyboardOctaveModifier += val;
    }
  }
}

int getVelocity() {
  return map(analogRead(A14), 0, 1023, 0, 127);
}

void onRelease(int val) {
  if(val >= 60) {
    if(digitalRead(modeSwitchPin) == HIGH) {
      usbMIDI.sendNoteOff(val + (12 * keyboardOctaveModifierTempArray[val - 48]), 0, 1);
    }    
    channelDisplayed = -1;
    keyboardOctaveModifierTempArray[val - 48] = -5;
  } 
}

void nextStep() {
  currentStep++;
  if(currentStep >= totalSteps) {
    currentStep = 0;
  }
}

void previousStep() {
  currentStep--;
  if(currentStep < 0) {
    currentStep = 15;
  }
}

void checkPixels() {
  for(int i = 0; i < 15; i++) {
    if(currentStep == i) {
      if(channelDisplayed >= 0) {
        if(velocityAtSteps[channelDisplayed][i] >= 0) {
          sequencerKeypad.pixels.setPixelColor(i, Adafruit_NeoPixel::ColorHSV(map(velocityAtSteps[channelDisplayed][i], 0, 127, 0, 54592), 255, 255));
        }   
      }
      else {
        sequencerKeypad.pixels.setPixelColor(i, 255, 255, 255);
      }
      sequencerKeypad.pixels.show();
    }
    else if(velocityAtSteps[channelDisplayed][i] >= 0) {
      sequencerKeypad.pixels.setPixelColor(i, Adafruit_NeoPixel::ColorHSV(map(velocityAtSteps[channelDisplayed][i], 0, 127, 0, 54592), 255, 87));
      sequencerKeypad.pixels.show();
    }
    else{
      boolean anyNoteAtStep = false;
      for(int j = 0; j < 25; j++) {
        if(velocityAtSteps[j][i] >= 0 && keyboardOctaveModifierAtSteps[j][i] >= -4) {
          anyNoteAtStep = true;
          break;
        }
      }
      if(anyNoteAtStep) {
        sequencerKeypad.pixels.setPixelColor(i, 87, 87, 87);
      }
      else {
        sequencerKeypad.pixels.setPixelColor(i, 0, 0, 0);
      } 
      sequencerKeypad.pixels.show();
    }
  } 
}

void checkPixel(int i) {
  if(currentStep == i) {
    if(channelDisplayed >= 0) {
      if(velocityAtSteps[channelDisplayed][currentStep] >= 0) {
        sequencerKeypad.pixels.setPixelColor(i, Adafruit_NeoPixel::ColorHSV(map(velocityAtSteps[channelDisplayed][i], 0, 127, 0, 54592), 255, 255));
      }   
    }
    else {
      sequencerKeypad.pixels.setPixelColor(i, 255, 255, 255);
    }
    sequencerKeypad.pixels.show();
  }
  else if(velocityAtSteps[channelDisplayed][i] >= 0) {
    sequencerKeypad.pixels.setPixelColor(i, Adafruit_NeoPixel::ColorHSV(map(velocityAtSteps[channelDisplayed][i], 0, 127, 0, 54592), 255, 87));
    sequencerKeypad.pixels.show();
  }
  else {
    boolean anyNoteAtStep = false;
    for(int j = 0; j < 25; j++) {
      if(velocityAtSteps[j][i] >= 0 && keyboardOctaveModifierAtSteps[j][i] >= -4) {
        anyNoteAtStep = true;
        break;
      }
    }
    if(anyNoteAtStep) {
      sequencerKeypad.pixels.setPixelColor(i, 87, 87, 87);
    }
    else {
      sequencerKeypad.pixels.setPixelColor(i, 0, 0, 0);
    } 
    sequencerKeypad.pixels.show();
  }
}


