#include "my_key_press.h"


KeyPressHandler::KeyPressHandler(int pin)
    : pin(pin), lastDebounceTime(0), debounceDelay(50), buttonState(HIGH), lastButtonState(HIGH) {}

void KeyPressHandler::begin() {
    pinMode(pin, INPUT_PULLUP); // 使用内部上拉电阻
}

bool KeyPressHandler::isPressed() {
    int reading = digitalRead(pin);

    // 检查按钮状态是否发生变化
    if (reading != lastButtonState) {
        // 记录最后一次变化的时间
        lastDebounceTime = millis();
    }

    // 如果按键状态保持不变超过debounceDelay，则认为是有效按键
    if ((millis() - lastDebounceTime) > debounceDelay) {
        // 更新buttonState到新的稳定状态
        if (reading != buttonState) {
            buttonState = reading;

            // 只有当按钮被按下时才触发事件
            if (buttonState == LOW) { // 因为使用INPUT_PULLUP，按下时为LOW
                Serial.println("Button pressed on pin " + String(pin));
                return true;
            }
        }
    }

    // 保存当前阅读状态作为lastButtonState
    lastButtonState = reading;

    return false;
}