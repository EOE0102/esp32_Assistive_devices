#ifndef MY_KEY_PRESS_H
#define MY_KEY_PRESS_H

#include <Arduino.h>

class KeyPressHandler {
public:
    KeyPressHandler(int pin);
    void begin();
    bool isPressed();

private:
    int pin;
    unsigned long lastDebounceTime;
    unsigned long debounceDelay;
    int buttonState;
    int lastButtonState;
};



#endif // MY_KEY_PRESS_H