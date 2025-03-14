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
#include "my_STT.h"

bool timeste = 0;
String stttext = "";
bool sttste = false;

// 讯飞STT 的key
String STTAPPID = "304358f2";
const char *STTAPISecret = "NjFmYjA2OWFiZTk4ZmE5OTU4NjUyZTgz";
const char *STTAPIKey = "6da7ae9a118c30d95a71b51ab122a9fd";



//QwenAPI
const char* apiKey = "sk-5554bc3278ef431e9099f5e05dbf6221";

// Send request to API
String inputText = "你好，通义千问！";
String apiUrl = "https://dashscope.aliyuncs.com/api/v1/services/aigc/text-generation/generation";





//Camera include
#include "my_camera.h"
#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
#include "camera_pins.h"



#include <ArduinoWebsockets.h>
using namespace websockets;
WebsocketsClient client;

// 向讯飞STT发送音频数据
void STTsend()
{
    uint8_t status = 0;
    int dataSize = 1280 * 8;
    int audioDataSize = recordingSize * 2;
    uint lan = (audioDataSize) / dataSize;
    uint lan_end = (audioDataSize) % dataSize;
    if (lan_end > 0)
    {
        lan++;
    }

    // Serial.printf("byteDatasize: %d , lan: %d , lan_end: %d \n", audioDataSize, lan, lan_end);
    String host_url = XF_wsUrl(STTAPISecret, STTAPIKey, "/v2/iat", "ws-api.xfyun.cn");
    Serial.println("Connecting to server.");
    bool connected = client.connect(host_url);
    if (connected)
    {
        Serial.println("Connected!");
    }
    else
    {
        Serial.println("Not Connected!");
    }
    // 分段向STT发送PCM音频数据
    for (int i = 0; i < lan; i++)
    {

        if (i == (lan - 1))
        {
        status = 2;
        }
        if (status == 0)
        {
        String input = "{";
        input += "\"common\":{ \"app_id\":\"" + STTAPPID + "\" },";
        input += "\"business\":{\"domain\": \"iat\", \"language\": \"zh_cn\", \"accent\": \"mandarin\", \"vinfo\":1,\"vad_eos\":10000},";
        input += "\"data\":{\"status\": 0, \"format\": \"audio/L16;rate=16000\",\"encoding\": \"raw\",\"audio\":\"";
        String base64audioString = base64::encode((uint8_t *)pcm_data, dataSize);
        input += base64audioString;
        input += "\"}}";
        Serial.printf("input: %d , status: %d \n", i, status);
        client.send(input);
        status = 1;
        }
        else if (status == 1)
        {
        String input = "{";
        input += "\"data\":{\"status\": 1, \"format\": \"audio/L16;rate=16000\",\"encoding\": \"raw\",\"audio\":\"";
        String base64audioString = base64::encode((uint8_t *)pcm_data + (i * dataSize), dataSize);
        input += base64audioString;
        input += "\"}}";
        // Serial.printf("input: %d , status: %d \n", i, status);
        client.send(input);
        }
        else if (status == 2)
        {
        if (lan_end == 0)
        {
            String input = "{";
            input += "\"data\":{\"status\": 2, \"format\": \"audio/L16;rate=16000\",\"encoding\": \"raw\",\"audio\":\"";
            String base64audioString = base64::encode((uint8_t *)pcm_data + (i * dataSize), dataSize);
            input += base64audioString;
            input += "\"}}";
            Serial.printf("input: %d , status: %d \n", i, status);
            client.send(input);
        }
        if (lan_end > 0)
        {
            String input = "{";
            input += "\"data\":{\"status\": 2, \"format\": \"audio/L16;rate=16000\",\"encoding\": \"raw\",\"audio\":\"";
            String base64audioString = base64::encode((uint8_t *)pcm_data + (i * dataSize), lan_end);
            input += base64audioString;
            input += "\"}}";
            Serial.printf("input: %d , status: %d \n", i, status);
            client.send(input);
        }
        }
        delay(30);
    }
}




String answer;
String getGPTAnswer(String inputText) {
    HTTPClient http;
    http.setTimeout(10000);
    http.begin(apiUrl);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String(apiKey));
    
    String roleContent = "你是一个辅助残疾人对话的大语言模型，你的名字是小曦。"
                        "你的主要工作是提供情绪价值，为用户提供各类对话服务，让用户开心。"
                        "回话的部分不要带有任何表情包。要求下面的回答严格控制在256字符以内。"
                        "如果用户表示需要结束对话的情况，你要说，好的，期待和你的下一次对话";
    String payload = R"({
        "model":"qwen-turbo",
        "input":{
            "messages":[
                {
                    "role": "system",
                    "content": ")" + roleContent + R"("
                },
                {
                    "role": "user",
                    "content": ")" + inputText + R"("
                }
            ]
        }
    })";
    
    int httpResponseCode = http.POST(payload);
    if (httpResponseCode == 200) {
        String response = http.getString();
        http.end();
        Serial.println(response);

        // Parse JSON response
        DynamicJsonDocument jsonDoc(1024);
        deserializeJson(jsonDoc, response);
        String outputText = jsonDoc["output"]["text"];
        return outputText;
        // Serial.println(outputText);
    } else {
        http.end();
        Serial.printf("Error %i \n", httpResponseCode);
        return "<error>";
    }
}










// WebSocket 消息回调函数
void onWebSocketMessage(WebsocketsMessage message) {
    Serial.print("Got Message: ");
    Serial.println(message.data());

    DynamicJsonDocument doc(1024); // 调整大小以适应你的 JSON 数据
    DeserializationError error = deserializeJson(doc, message.data());
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    JsonArray ws = doc["data"]["result"]["ws"].as<JsonArray>();
    for (JsonObject word : ws) {
        int bg = word["bg"];
        const char *w = word["cw"][0]["w"];
        stttext += w;
    }

    if (doc["data"]["status"] == 2) { // 收到结束标志
        sttste = true;
        Serial.print("语音识别：");
        Serial.println(stttext);
    }
}









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

    setup_ntp_client();
    getDateTime();

    Serial.println("Setup I2S ...");
    i2s_install();
    i2s_setpin();
    esp_err_t err = i2s_start(I2S_PORT_0);
    if (err != ESP_OK)
    {
        Serial.printf("I2S start failed (I2S_PORT_0): %d\n", err);
        while (true)
        ;
    }

    client.onMessage(onWebSocketMessage);

}

// the loop function runs over and over again forever
void loop() {
    // 添加一个小延迟以避免CPU占用过高
    delay(10);

    // 非阻塞LED闪烁
    board_led_blink_nonblocking(boardLED, boardLEDTimerPrepare);


    // 检查是否有数据可读
    if (Serial.available() > 0) {
        char command = Serial.read();
        if (command == '1') {
            board_led_blink_nonblocking(boardLED, boardLEDTimer);
            Serial.printf("Start recognition\r\n\r\n");



            stttext = "";
            Serial.println("Recording...");
            size_t bytes_read = 0;
            recordingSize = 0;
            // 分配 pcm_data
            pcm_data = (int16_t *)ps_malloc(BUFFER_SIZE * sizeof(int16_t));
            if (!pcm_data)
            {
                Serial.println("Failed to allocate memory for pcm_data from PSRAM");
                return;
            }

            // uint16_t x = 0, y = 0;
            while (recordingSize < recordTimeSeconds* SAMPLE_RATE)
            { // 开始循环录音，将录制结果保存在pcm_data中
                esp_err_t result = i2s_read(I2S_PORT_0, audioData, sizeof(audioData), &bytes_read, portMAX_DELAY);
                memcpy(pcm_data + recordingSize, audioData, bytes_read);
                recordingSize += bytes_read / 2;
            }

            Serial.printf("Total bytes read: %d\n", recordingSize);
            Serial.println("Recording complete.");
            STTsend(); // STT请求开始
            free(pcm_data);

        }
    }

    if (client.available())
    {
        client.poll();
    }

    if (sttste) {
        // 处理 stttext
        Serial.print("Processing recognized text: ");
        Serial.println(stttext);

        // 在这里调用后续函数，例如发送到 GPT 或其他处理
        String gptAnswer = getGPTAnswer(stttext);
        Serial.print("GPT Answer: ");
        Serial.println(gptAnswer);

        // 重置标志
        sttste = false;
        stttext = ""; // 清空 stttext 以便下次使用
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




