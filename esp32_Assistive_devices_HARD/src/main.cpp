#include <Arduino.h>


//my function
//LED
#include "my_LED.h"
#define boardLED LED_BUILTIN
#define boardLEDTimer 100

//连接Wifi
#include "my_WiFi.h"
const char* ssid = "XuHandy";
const char* password = "qwer1234";
String serverTime = "";

//测试按键 消抖Key
#include "my_key_press.h"
#define inputKeyPin A0
KeyPressHandler inputKeyPinHandler(inputKeyPin); // 创建按键处理对象

//Camera include
#include "my_camera.h"
#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
#include "camera_pins.h"



//SmartAssistant-board code
//Audio Record
#include "Audio_Record.h"
// 定义麦克风引脚
#define PIN_I2S_BCLK 2  // 时钟线，对应INMP441的SCK
#define PIN_I2S_LRC  1  // 声道选择线，对应INMP441的WS，由主机发送给从机，WS低电平时，从机发送左声道数据，高电平时发送右声道数据
#define PIN_I2S_DIN 42  // 数据线，对应INMP441的SD
Audio_Record audio_Record(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DIN);










// the setup function runs once when you press reset or power the board
void setup() {
    // 初始化串口通信，波特率为115200
    Serial.begin(115200);

    // 初始化数字引脚 LED_BUILTIN
    pinMode(LED_BUILTIN, OUTPUT);

    // 连接到Wi-Fi网络
    connectToWiFi(ssid, password);

    Serial.println(getTimeFromServer());
    // 检查并打印服务器时间
    serverTime = checkAndPrintServerTime(ssid, password);

}

// the loop function runs over and over again forever
void loop() {
    // 添加一个小延迟以避免CPU占用过高
    delay(10);

    // 非阻塞LED闪烁
    board_led_blink_nonblocking(boardLED, boardLEDTimer);


    // 检查是否有数据可读
    if (Serial.available() > 0) {
        char command = Serial.read();
        if (command == '1') {
            Serial.printf("Start recognition\r\n\r\n");
        }
    }

    // 更新按键状态
    inputKeyPinHandler.begin();
    // 检查按键是否被按下
    if (inputKeyPinHandler.isPressed()) {
        Serial.println("inputKeyPin pressed");
    }


}

















