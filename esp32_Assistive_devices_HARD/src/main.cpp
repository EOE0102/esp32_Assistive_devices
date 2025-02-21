#include <Arduino.h>


//my function
#include "my_LED.h"
#include "my_WiFi.h"
#include "my_key_press.h"

#include "my_camera.h"

//测试LED
#define testLed LED_BUILTIN

//测试按键 消抖Key
#define inputKeyPin A0
#define inputKeyPin_old A0

KeyPressHandler keyPressHandler(inputKeyPin); // 创建按键处理对象

//连接Wifi
const char* ssid = "XuHandy";
const char* password = "qwer1234";
String serverTime = "";



//Camera include
#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
#include "camera_pins.h"




// the setup function runs once when you press reset or power the board
void setup() {
    // 初始化串口通信，波特率为115200
    Serial.begin(115200);

    // 初始化数字引脚 LED_BUILTIN
    pinMode(LED_BUILTIN, OUTPUT);

    //Pinmode
    pinMode(inputKeyPin_old, INPUT_PULLUP);




    // 连接到Wi-Fi网络
    connectToWiFi(ssid, password);

    Serial.println(getTimeFromServer());
    // 检查并打印服务器时间
    serverTime = checkAndPrintServerTime(ssid, password);

}

// the loop function runs over and over again forever
void loop() {
    


    // 检查是否有数据可读
    if (Serial.available() > 0) {
        char command = Serial.read();
        if (command == '1') {
            Serial.printf("Start recognition\r\n\r\n");
        }
    }

    // 更新按键状态
    keyPressHandler.update();
    // 检查按键是否被按下

    if (keyPressHandler.isPressed()) {
        Serial.println("inputKeyPin pressed");
    }


    if (digitalRead(inputKeyPin_old) == LOW) {
        Serial.println("inputKeyPin_old pressed");
        delay(20);
    }


    // 非阻塞LED闪烁
    board_led_blink_nonblocking(testLed, 100);

    // 添加一个小延迟以避免CPU占用过高
    delay(10);



}

















