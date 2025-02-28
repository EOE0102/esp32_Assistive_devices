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



//Camera include
#include "my_camera.h"
#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
#include "camera_pins.h"

//WEB
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
//WEB
using namespace websockets;

WebsocketsClient webSocketClient_Record;
void onEventsCallback_Record(WebsocketsEvent event, String data);
void onMessageCallback_Record(WebsocketsMessage message);
void ConnServer_Record();



//SmartAssistant-board code
//Audio Record
#include "Audio_Record.h"
// 定义麦克风引脚
#define PIN_I2S_BCLK 42  // 时钟线，对应INMP441的SCK
#define PIN_I2S_LRC  46  // 声道选择线，对应INMP441的WS，由主机发送给从机，WS低电平时，从机发送左声道数据，高电平时发送右声道数据
#define PIN_I2S_DIN 41  // 数据线，对应INMP441的SD
Audio_Record audio_Record(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DIN);










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



    // 初始化音频录音模块audio_Record
    audio_Record.init();

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
        while(1){
            audio_Record.Record();
            // 计算音频数据的RMS值
            float rms = audio_Record.calculateRMS((uint8_t *)audio_Record.wavData[0], 1280);
            Serial.print("rms=");
            Serial.println(rms , 3);

            if(digitalRead(inputKeyPin)==HIGH){
                break;
            }
        }
    }
}






int noise = 50;
// 讯飞stt语种设置
String language = "zh_cn";     //zh_cn：中文（支持简单的英文识别）en_us：English
String APPID = "304358f2"; // 自己的星火大模型账号参数
String APIKey = "6da7ae9a118c30d95a71b51ab122a9fd";
String APISecret = "NjFmYjA2OWFiZTk4ZmE5OTU4NjUyZTgz";
String appId_Record = APPID;



void onEventsCallback_Record(WebsocketsEvent event, String data)
{
    // 当WebSocket连接打开时触发
    if (event == WebsocketsEvent::ConnectionOpened)
    {
        // 向串口输出提示信息
        Serial.println("Send message to xunfeiyun");
        // digitalWrite(led2, HIGH);

        // 初始化变量
        int silence = 0;
        int firstframe = 1;
        int j = 0;
        int voicebegin = 0;
        int voice = 0;

        // 创建一个静态JSON文档对象，2000一般够了，不够可以再加（最多不能超过4096），但是可能会发生内存溢出
        // DynamicJsonDocument doc(2500);
        StaticJsonDocument<2000> doc;

        Serial.println("开始录音");
        while (1)
        {
            // 清空JSON文档
            doc.clear();
            // 创建data对象
            JsonObject data = doc.createNestedObject("data");
            // 录制音频数据
            audio_Record.Record();
            // 计算音频数据的RMS值
            float rms = audio_Record.calculateRMS((uint8_t *)audio_Record.wavData[0], 1280);
            // printf("%d %f\n", 0, rms);

            // 判断是否为噪音
            if (rms < noise) 
            {
                if (voicebegin == 1)
                {
                    silence++;
                    // Serial.print("noise:");
                    // Serial.println(noise);
                }
            }
            else
            {
                voice++;
                if (voice >= 5)
                {
                    voicebegin = 1;
                }
                else
                {
                    voicebegin = 0;
                }
                silence = 0;
            }

            // 如果静音达到6个周期，发送结束标志的音频数据
            if (silence == 6)
            {
                data["status"] = 2;
                data["format"] = "audio/L16;rate=8000";
                data["audio"] = base64::encode((byte *)audio_Record.wavData[0], 1280);
                data["encoding"] = "raw";
                j++;

                String jsonString;
                serializeJson(doc, jsonString);

                webSocketClient_Record.send(jsonString);
                // Serial.println(jsonString);
                // digitalWrite(led2, LOW);
                delay(40);
                Serial.println("录音结束");
                break;
            }

            // 处理第一帧音频数据
            if (firstframe == 1)
            {
                data["status"] = 0;
                data["format"] = "audio/L16;rate=8000";
                data["audio"] = base64::encode((byte *)audio_Record.wavData[0], 1280);
                data["encoding"] = "raw";
                j++;

                JsonObject common = doc.createNestedObject("common");
                common["app_id"] = appId_Record.c_str();

                JsonObject business = doc.createNestedObject("business");
                business["domain"] = "iat";
                business["language"] = language.c_str();
                business["accent"] = "mandarin";
                // 不使用动态修正
                // business["vinfo"] = 1;
                // 使用动态修正
                business["dwa"] = "wpgs";
                business["vad_eos"] = 1000;

                String jsonString;
                serializeJson(doc, jsonString);

                webSocketClient_Record.send(jsonString);
                // Serial.println("处理第一帧音频数据"+jsonString);
                firstframe = 0;
                delay(40);
            }
            else
            {
                // 处理后续帧音频数据
                data["status"] = 1;
                data["format"] = "audio/L16;rate=8000";
                data["audio"] = base64::encode((byte *)audio_Record.wavData[0], 1280);
                data["encoding"] = "raw";

                String jsonString;
                serializeJson(doc, jsonString);

                webSocketClient_Record.send(jsonString);
                // Serial.println("处理后续帧音频数据"+jsonString);
                delay(40);
            }
        }
    }
    // 当WebSocket连接关闭时触发
    else if (event == WebsocketsEvent::ConnectionClosed)
    {
        Serial.println("Connnection1 Closed");
    }
    // 当收到Ping消息时触发
    else if (event == WebsocketsEvent::GotPing)
    {
        Serial.println("Got a Ping!");
    }
    // 当收到Pong消息时触发
    else if (event == WebsocketsEvent::GotPong)
    {
        Serial.println("Got a Pong!");
    }
}


String url_Record = "";

void ConnServer_Record()
{
    Serial.println("url_Record:" + url_Record);
    webSocketClient_Record.onMessage(onMessageCallback_Record);
    webSocketClient_Record.onEvent(onEventsCallback_Record);
    // Connect to WebSocket
    Serial.println("开始连接讯飞STT语音转文字服务......Begin connect to server1(Xunfei STT)......");
    if (webSocketClient_Record.connect(url_Record.c_str()))
    {
        Serial.println("连接成功！Connected to server1(Xunfei STT)!");
    }
    else
    {
        Serial.println("连接失败！Failed to connect to server1(Xunfei STT)!");
    }
}

String askquestion = "";

void onMessageCallback_Record(WebsocketsMessage message)
{
    StaticJsonDocument<4096> jsonDocument;
    DeserializationError error = deserializeJson(jsonDocument, message.data());

    if (!error)
    {
        int code = jsonDocument["code"];
        if (code != 0)
        {
            Serial.println(code);
            Serial.println(message.data());
            webSocketClient_Record.close();
        }
        else
        {
            Serial.println("xunfeiyun return message:");
            Serial.println(message.data());
            JsonArray ws = jsonDocument["data"]["result"]["ws"].as<JsonArray>();

            for (JsonVariant i : ws)
            {
                for (JsonVariant w : i["cw"].as<JsonArray>())
                {
                    askquestion += w["w"].as<String>();
                }
            }
            Serial.println(askquestion);
            int status = jsonDocument["data"]["status"];
            if (status == 2)
            {
                Serial.println("status == 2");
                webSocketClient_Record.close();
                if (askquestion == "")
                {
                    askquestion = "sorry, i can't hear you";
                }
                else
                {
                    Serial.println(error.c_str());
                }
            }
        }
    }
    else
    {
        Serial.println("error:");
        Serial.println(error.c_str());
        Serial.println(message.data());
    }
}








