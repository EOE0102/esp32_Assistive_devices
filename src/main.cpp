#include <Arduino.h>


//my function
//LED
#include "my_LED.h"
#define boardLED LED_BUILTIN
#define boardLEDTimerPrepare 1000
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

//录音相关
#include "my_I2S.h"
#define I2S_WS D9
#define I2S_SD D3
#define I2S_SCK D10
#define I2S_PORT_0 I2S_NUM_0
#define SAMPLE_RATE 16000
#define RECORD_TIME_SECONDS 10
#define BUFFER_SIZE (SAMPLE_RATE * RECORD_TIME_SECONDS)

#define CHUNK_SIZE 2048
const int recordTimeSeconds = 3;//录音时间秒为单位
int16_t audioData[2560];
int16_t *pcm_data; // 录音缓存区
uint recordingSize = 0;

// char* psramBuffer = (char*)ps_malloc(512000);
String odl_answer;

String answer_list[10];
uint8_t answer_list_num = 0;
bool answer_ste = 0;


//讯飞TTS相关
#include "Base64_Arturo.h"

// 讯飞STT 的key
String STTAPPID = "304358f2";
const char *STTAPISecret = "NjFmYjA2OWFiZTk4ZmE5OTU4NjUyZTgz";
const char *STTAPIKey = "6da7ae9a118c30d95a71b51ab122a9fd";







//Camera include
#include "my_camera.h"
#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
#include "camera_pins.h"











// the setup function runs once when you press reset or power the board
void setup() {
    // 初始化串口通信，波特率为115200
    Serial.begin(115200);

    // 初始化数字引脚 LED_BUILTIN
    pinMode(LED_BUILTIN, OUTPUT);
    board_led_blink_nonblocking(boardLED, boardLEDTimerPrepare);
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
        // 录制音频数据
    }
    
    if (digitalRead(inputKeyPin)==LOW){
        Serial.print("2333");

    }
}




