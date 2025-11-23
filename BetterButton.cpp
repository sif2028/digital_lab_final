#include "Arduino.h"
#include "BetterButton.h"

BetterButton::BetterButton(int _buttonPin, int _val)
{
  buttonPin = _buttonPin;
  val = _val;
  pinMode(buttonPin, INPUT);
}

void BetterButton::process()
{
  lastButtonState = buttonState;
  buttonState = digitalRead(buttonPin);

  if (lastButtonState == LOW && buttonState == HIGH) {
    if (pressCallback != NULL) {
      pressCallback(val);
    }
    delay(5);
  }
  if (lastButtonState == HIGH && buttonState == LOW) {
    if(releaseCallback != NULL) {
      releaseCallback(val);
    }
    delay(5);
  }
}

void BetterButton::pressHandler(void (*f)(int))    //button down
{
  pressCallback = *f;
}

void BetterButton::releaseHandler(void (*f)(int))  //button release
{
  releaseCallback = *f;
}
