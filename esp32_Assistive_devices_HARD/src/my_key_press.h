#ifndef MY_KEY_PRESS_H
#define MY_KEY_PRESS_H

#include <Arduino.h>
#include <Bounce2.h>

class KeyPressHandler {
public:
    KeyPressHandler(int pin);
    void update();
    bool isPressed() const;

private:
    Bounce debouncer;
};

#endif // MY_KEY_PRESS_H