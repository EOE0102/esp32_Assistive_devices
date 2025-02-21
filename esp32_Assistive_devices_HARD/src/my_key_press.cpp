#include "my_key_press.h"

KeyPressHandler::KeyPressHandler(int pin) {
    // 初始化按键引脚为输入上拉模式
    pinMode(pin, INPUT_PULLUP);
    // 将按键引脚附加到debouncer对象
    debouncer.attach(pin);
    // 设置去抖动间隔时间为25毫秒
    debouncer.interval(25);
}

void KeyPressHandler::update() {
    // 更新去抖动状态
    debouncer.update();
}

bool KeyPressHandler::isPressed() const {
    // 检查按键是否被按下（从高电平变为低电平）
    return debouncer.fell();
}