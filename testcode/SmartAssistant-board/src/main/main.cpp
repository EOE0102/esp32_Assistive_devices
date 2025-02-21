#include <Arduino.h>
#include "base64.h"
#include "WiFi.h"
#include <WiFiClientSecure.h>
#include "HTTPClient.h"
#include "Audio1.h"
#include "Audio2.h"
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>


//SD卡测试
// #include "ESP_I2S.h"
// #include "FS.h"
// #include "SD.h"

using namespace websockets;

#define key A0
#define ADC 32
#define led3 LED_BUILTIN
#define led2 D5
#define led1 D4
const char *wifiData[][2] = {
    {"XuHandy", "qwer1234"}, // 替换为自己常用的wifi名和密码
    // {"222", "12345678"},
    // 继续添加需要的 Wi-Fi 名称和密码
};

String APPID = "304358f2"; // 自己的星火大模型账号参数
String APIKey = "6da7ae9a118c30d95a71b51ab122a9fd";
String APISecret = "NjFmYjA2OWFiZTk4ZmE5OTU4NjUyZTgz";
String appId1 = APPID;
String domain1 = "4.0Ultra";    // 根据需要更改
String websockets_server = "ws://spark-api.xf-yun.com/v4.0/chat";   // 根据需要更改
String websockets_server1 = "ws://iat-api.xfyun.cn/v2/iat";
// 讯飞stt语种设置
String language = "zh_cn";     //zh_cn：中文（支持简单的英文识别）en_us：English

bool ledstatus = true;
bool startPlay = false;
bool lastsetence = false;
bool isReady = false;
unsigned long urlTime = 0;
unsigned long pushTime = 0;
int mainStatus = 0;
int receiveFrame = 0;
int noise = 50;
HTTPClient https;

hw_timer_t *timer = NULL;

uint8_t adc_start_flag = 0;
uint8_t adc_complete_flag = 0;

Audio1 audio1;
Audio2 audio2(false, 3, I2S_NUM_1);

// XIAO-ESP32-S3-SENSE定义音频放大模块的I2S引脚定义，喇叭
#define I2S_DOUT A3 // DIN
#define I2S_BCLK A2 // BCLK
#define I2S_LRC A1  // LRC


void gain_token(void);
void getText(String role, String content);
void checkLen(JsonArray textArray);
int getLength(JsonArray textArray);
float calculateRMS(uint8_t *buffer, int bufferSize);
void ConnServer();
void ConnServer1();
void loop_temp();

DynamicJsonDocument doc(4000);
JsonArray text = doc.to<JsonArray>();

String url = "";
String url1 = "";
String Date = "";
DynamicJsonDocument gen_params(const char *appid, const char *domain);

String askquestion = "";
String Answer = "";

using namespace websockets;

WebsocketsClient webSocketClient;
WebsocketsClient webSocketClient1;

int loopcount = 0;
void onMessageCallback(WebsocketsMessage message)
{
    StaticJsonDocument<4096> jsonDocument;
    DeserializationError error = deserializeJson(jsonDocument, message.data());

    if (!error)
    {
        int code = jsonDocument["header"]["code"];
        if (code != 0)
        {
            Serial.print("sth is wrong: ");
            Serial.println(code);
            Serial.println(message.data());
            webSocketClient.close();
        }
        else
        {
            receiveFrame++;
            Serial.print("receiveFrame:");
            Serial.println(receiveFrame);
            JsonObject choices = jsonDocument["payload"]["choices"];
            int status = choices["status"];
            const char *content = choices["text"][0]["content"];
            Serial.println(content);
            Answer += content;
            String answer = "";
            if (Answer.length() >= 120 && (audio2.isplaying == 0))
            {
                String subAnswer = Answer.substring(0, 120);
                Serial.print("subAnswer:");
                Serial.println(subAnswer);
                int lastPeriodIndex = subAnswer.lastIndexOf("。");

                if (lastPeriodIndex != -1)
                {
                    answer = Answer.substring(0, lastPeriodIndex + 1);
                    Serial.print("answer: ");
                    Serial.println(answer);
                    Answer = Answer.substring(lastPeriodIndex + 2);
                    Serial.print("Answer: ");
                    Serial.println(Answer);
                    audio2.connecttospeech(answer.c_str(), "zh");
                }
                else
                {
                    const char *chinesePunctuation = "？，：；,.";

                    int lastChineseSentenceIndex = -1;

                    for (int i = 0; i < Answer.length(); ++i)
                    {
                        char currentChar = Answer.charAt(i);

                        if (strchr(chinesePunctuation, currentChar) != NULL)
                        {
                            lastChineseSentenceIndex = i;
                        }
                    }
                    if (lastChineseSentenceIndex != -1)
                    {
                        answer = Answer.substring(0, lastChineseSentenceIndex + 1);
                        audio2.connecttospeech(answer.c_str(), "zh");
                        Answer = Answer.substring(lastChineseSentenceIndex + 2);
                    }
                    else
                    {
                        answer = Answer.substring(0, 120);
                        audio2.connecttospeech(answer.c_str(), "zh");
                        Answer = Answer.substring(120 + 1);
                    }
                }
                startPlay = true;
            }

            if (status == 2)
            {
                getText("assistant", Answer);
                if (Answer.length() <= 80 && (audio2.isplaying == 0))
                {
                    // getText("assistant", Answer);
                    audio2.connecttospeech(Answer.c_str(), "zh");
                }
            }
        }
    }
}

void onEventsCallback(WebsocketsEvent event, String data)
{
    if (event == WebsocketsEvent::ConnectionOpened)
    {
        Serial.println("Send message to server0!");
        DynamicJsonDocument jsonData = gen_params(appId1.c_str(), domain1.c_str());
        String jsonString;
        serializeJson(jsonData, jsonString);
        Serial.println(jsonString);
        webSocketClient.send(jsonString);
    }
    else if (event == WebsocketsEvent::ConnectionClosed)
    {
        Serial.println("Connnection0 Closed");
    }
    else if (event == WebsocketsEvent::GotPing)
    {
        Serial.println("Got a Ping!");
    }
    else if (event == WebsocketsEvent::GotPong)
    {
        Serial.println("Got a Pong!");
    }
}

void onMessageCallback1(WebsocketsMessage message)
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
            webSocketClient1.close();
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
                webSocketClient1.close();
                if (askquestion == "")
                {
                    askquestion = "sorry, i can't hear you";
                    audio2.connecttospeech(askquestion.c_str(), "zh");
                }
                else
                {
                    getText("user", askquestion);
                    Serial.print("text:");
                    Serial.println(text);
                    Answer = "";
                    lastsetence = false;
                    isReady = true;
                    ConnServer();
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

void onEventsCallback1(WebsocketsEvent event, String data)
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
            audio1.Record();
            // 计算音频数据的RMS值
            float rms = calculateRMS((uint8_t *)audio1.wavData[0], 1280);
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
                data["audio"] = base64::encode((byte *)audio1.wavData[0], 1280);
                data["encoding"] = "raw";
                j++;

                String jsonString;
                serializeJson(doc, jsonString);

                webSocketClient1.send(jsonString);
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
                data["audio"] = base64::encode((byte *)audio1.wavData[0], 1280);
                data["encoding"] = "raw";
                j++;

                JsonObject common = doc.createNestedObject("common");
                common["app_id"] = appId1.c_str();

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

                webSocketClient1.send(jsonString);
                // Serial.println("处理第一帧音频数据"+jsonString);
                firstframe = 0;
                delay(40);
            }
            else
            {
                // 处理后续帧音频数据
                data["status"] = 1;
                data["format"] = "audio/L16;rate=8000";
                data["audio"] = base64::encode((byte *)audio1.wavData[0], 1280);
                data["encoding"] = "raw";

                String jsonString;
                serializeJson(doc, jsonString);

                webSocketClient1.send(jsonString);
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

void ConnServer()
{
    Serial.println("url:" + url);

    webSocketClient.onMessage(onMessageCallback);
    webSocketClient.onEvent(onEventsCallback);
    // 开始连接WebSocket服务器
    Serial.println("开始连接讯飞星火大模型服务......Begin connect to server0(Xunfei Spark LLM)......");
    if (webSocketClient.connect(url.c_str()))
    {
        // 如果连接成功，输出成功信息
        Serial.println("连接LLM成功！Connected to server0(Xunfei Spark LLM)!");
    }
    else
    {
        // 如果连接失败，输出失败信息
        Serial.println("连接LLM失败！Failed to connect to server0(Xunfei Spark LLM)!");
    }
}

void ConnServer1()
{
    Serial.println("url1:" + url1);
    webSocketClient1.onMessage(onMessageCallback1);
    webSocketClient1.onEvent(onEventsCallback1);
    // Connect to WebSocket
    Serial.println("开始连接讯飞STT语音转文字服务......Begin connect to server1(Xunfei STT)......");
    if (webSocketClient1.connect(url1.c_str()))
    {
        Serial.println("连接成功！Connected to server1(Xunfei STT)!");
    }
    else
    {
        Serial.println("连接失败！Failed to connect to server1(Xunfei STT)!");
    }
}

void voicePlay()
{
    if ((audio2.isplaying == 0) && Answer != "")
    {
        // String subAnswer = "";
        // String answer = "";
        // if (Answer.length() >= 100)
        //     subAnswer = Answer.substring(0, 100);
        // else
        // {
        //     subAnswer = Answer.substring(0);
        //     lastsetence = true;
        //     // startPlay = false;
        // }

        // Serial.print("subAnswer:");
        // Serial.println(subAnswer);
        int firstPeriodIndex = Answer.indexOf("。");
        int secondPeriodIndex = 0;

        if (firstPeriodIndex != -1)
        {
            secondPeriodIndex = Answer.indexOf("。", firstPeriodIndex + 1);
            if (secondPeriodIndex == -1)
                secondPeriodIndex = firstPeriodIndex;
        }
        else
        {
            secondPeriodIndex = firstPeriodIndex;
        }

        if (secondPeriodIndex != -1)
        {
            String answer = Answer.substring(0, secondPeriodIndex + 1);
            Serial.print("answer: ");
            Serial.println(answer);
            Answer = Answer.substring(secondPeriodIndex + 2);
            audio2.connecttospeech(answer.c_str(), "zh");
        }
        else
        {
            const char *chinesePunctuation = "？，：；,.";

            int lastChineseSentenceIndex = -1;

            for (int i = 0; i < Answer.length(); ++i)
            {
                char currentChar = Answer.charAt(i);

                if (strchr(chinesePunctuation, currentChar) != NULL)
                {
                    lastChineseSentenceIndex = i;
                }
            }

            if (lastChineseSentenceIndex != -1)
            {
                String answer = Answer.substring(0, lastChineseSentenceIndex + 1);
                audio2.connecttospeech(answer.c_str(), "zh");
                Answer = Answer.substring(lastChineseSentenceIndex + 2);
            }
        }
        startPlay = true;
    }
    else
    {
        // digitalWrite(led3, LOW);
    }
}

void wifiConnect(const char *wifiData[][2], int numNetworks)
{
    WiFi.disconnect(true);
    for (int i = 0; i < numNetworks; ++i)
    {
        const char *ssid = wifiData[i][0];
        const char *password = wifiData[i][1];

        Serial.print("Connecting to ");
        Serial.println(ssid);

        WiFi.begin(ssid, password);
        uint8_t count = 0;
        while (WiFi.status() != WL_CONNECTED)
        {
            digitalWrite(led1, ledstatus);
            ledstatus = !ledstatus;
            Serial.print(".");
            count++;
            if (count >= 30)
            {
                Serial.printf("\r\n-- wifi connect fail! --");
                break;
            }
            vTaskDelay(100);
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.printf("\r\n-- wifi connect success! --\r\n");
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
            Serial.println("Free Heap: " + String(ESP.getFreeHeap()));
            return; // 如果连接成功，退出函数
        }
    }
}

// 拼接讯飞websocket鉴权参数
String getUrl(String Spark_url, String host, String path, String Date)
{

    // 拼接签名原始字符串
    String signature_origin = "host: " + host + "\n";
    signature_origin += "date: " + Date + "\n";
    signature_origin += "GET " + path + " HTTP/1.1";
    // signature_origin="host: spark-api.xf-yun.com\ndate: Mon, 04 Mar 2024 19:23:20 GMT\nGET /v3.5/chat HTTP/1.1";

    // 使用 HMAC-SHA256 进行加密
    unsigned char hmac[32];                                 // 存储HMAC结果
    mbedtls_md_context_t ctx;                               // HMAC上下文
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;          // 使用SHA256哈希算法
    const size_t messageLength = signature_origin.length(); // 签名原始字符串的长度
    const size_t keyLength = APISecret.length();            // 密钥的长度

    // 初始化HMAC上下文
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
    // 设置HMAC密钥
    mbedtls_md_hmac_starts(&ctx, (const unsigned char *)APISecret.c_str(), keyLength);
    // 更新HMAC上下文
    mbedtls_md_hmac_update(&ctx, (const unsigned char *)signature_origin.c_str(), messageLength);
    // 完成HMAC计算
    mbedtls_md_hmac_finish(&ctx, hmac);
    // 释放HMAC上下文
    mbedtls_md_free(&ctx);

    // base64 编码
    String signature_sha_base64 = base64::encode(hmac, sizeof(hmac) / sizeof(hmac[0]));

    // 替换Date字符串中的特殊字符
    Date.replace(",", "%2C");
    Date.replace(" ", "+");
    Date.replace(":", "%3A");

    // 构建Authorization原始字符串
    String authorization_origin = "api_key=\"" + APIKey + "\", algorithm=\"hmac-sha256\", headers=\"host date request-line\", signature=\"" + signature_sha_base64 + "\"";
    // 将Authorization原始字符串进行Base64编码
    String authorization = base64::encode(authorization_origin);
    // 构建最终的URL
    String url = Spark_url + '?' + "authorization=" + authorization + "&date=" + Date + "&host=" + host;
    // 向串口输出生成的URL
    Serial.println(url);
    // 返回生成的URL
    return url;
}

void getTimeFromServer()
{
    String timeurl = "https://www.baidu.com";
    HTTPClient http;
    http.begin(timeurl);
    const char *headerKeys[] = {"Date"};
    http.collectHeaders(headerKeys, sizeof(headerKeys) / sizeof(headerKeys[0]));
    int httpCode = http.GET();
    Date = http.header("Date");
    Serial.println(Date);
    http.end();
    // delay(50); // 可以根据实际情况调整延时时间
    //Thu, 20 Feb 2025 08:45:38 GMT
}

void setup()
{
    // String Date = "Fri, 22 Mar 2024 03:35:56 GMT";
    Serial.begin(115200);
    // pinMode(ADC,ANALOG);

    pinMode(key, INPUT_PULLUP);
    // pinMode(34, INPUT_PULLUP);
    // pinMode(35, INPUT_PULLUP);
    pinMode(led1, OUTPUT);
    pinMode(led2, OUTPUT);
    pinMode(led3, OUTPUT);
    audio1.init();
    Serial.println("audio1.init()");

    int numNetworks = sizeof(wifiData) / sizeof(wifiData[0]);
    wifiConnect(wifiData, numNetworks);

    getTimeFromServer();

    audio2.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    Serial.println("audio2.setPinout");

    audio2.setVolume(50);
    Serial.println("setVolume(50)");

    // // String Date = "Fri, 22 Mar 2024 03:35:56 GMT";
    url = getUrl(websockets_server, "spark-api.xf-yun.com", websockets_server.substring(25), Date);
    url1 = getUrl(websockets_server1, "iat-api.xfyun.cn", "/v2/iat", Date);
    urlTime = millis();
    Serial.println("urlTime = millis();");
    ///////////////////////////////////
}



void loop()
{
    // 轮询处理WebSocket客户端消息
    webSocketClient.poll(); // 必须定期调用！
    webSocketClient1.poll();// 必须定期调用！
    delay(10);
    // 其他非阻塞任务（如传感器读取、UI刷新）
    // 如果有多段语音需要播放
    if (startPlay)
    {
        voicePlay();
    }

    // 音频处理循环
    audio2.loop();

    // 如果音频正在播放
    if (audio2.isplaying == 1)
    {
        digitalWrite(led3, HIGH);// 点亮板载LED指示灯
    }
    else
    {
        digitalWrite(led3, LOW);// 熄灭板载LED指示灯
        if ((urlTime + 240000 < millis()) && (audio2.isplaying == 0))
        {
            urlTime = millis();
            getTimeFromServer();
            url = getUrl(websockets_server, "spark-api.xf-yun.com", websockets_server.substring(25), Date);
            url1 = getUrl(websockets_server1, "iat-api.xfyun.cn", "/v2/iat", Date);
        }
    }

    if (digitalRead(key) == 0)
    {
        audio2.isplaying = 0;
        startPlay = false;
        isReady = false;
        Answer = "";
        Serial.println("Answer:"+ Answer);
        Serial.println("Start recognition\r\n\r\n");

        adc_start_flag = 1;
        // Serial.println(esp_get_free_heap_size());

        if (urlTime + 240000 < millis()) // 超过4分钟，重新做一次鉴权
        {
            urlTime = millis();
            getTimeFromServer();
            url = getUrl(websockets_server, "spark-api.xf-yun.com", websockets_server.substring(25), Date);
            url1 = getUrl(websockets_server1, "iat-api.xfyun.cn", "/v2/iat", Date);
        }
        askquestion = "";
        // audio2.connecttospeech(askquestion.c_str(), "zh");
        // 连接到WebSocket服务器1讯飞stt
        ConnServer1();
        // ConnServer();
        // delay(6000);
        // audio1.Record();
        adc_complete_flag = 0;

        // Serial.println(text);
        // checkLen(text);
    }
}

void getText(String role, String content)
{
    checkLen(text);
    DynamicJsonDocument jsoncon(1024);
    jsoncon["role"] = role;
    jsoncon["content"] = content;
    text.add(jsoncon);
    jsoncon.clear();
    String serialized;
    serializeJson(text, serialized);
    Serial.print("text: ");
    Serial.println(serialized);
    // serializeJsonPretty(text, Serial);
}

int getLength(JsonArray textArray)
{
    int length = 0;
    for (JsonObject content : textArray)
    {
        const char *temp = content["content"];
        int leng = strlen(temp);
        length += leng;
    }
    return length;
}

void checkLen(JsonArray textArray)
{
    while (getLength(textArray) > 3000)
    {
        textArray.remove(0);
    }
    // return textArray;
}

DynamicJsonDocument gen_params(const char *appid, const char *domain)
{
    DynamicJsonDocument data(2048);

    JsonObject header = data.createNestedObject("header");
    header["app_id"] = appid;
    header["uid"] = "1234";

    JsonObject parameter = data.createNestedObject("parameter");
    JsonObject chat = parameter.createNestedObject("chat");
    chat["domain"] = domain;
    chat["temperature"] = 0.5;
    chat["max_tokens"] = 1024;

    JsonObject payload = data.createNestedObject("payload");
    JsonObject message = payload.createNestedObject("message");

    JsonArray textArray = message.createNestedArray("text");
    for (const auto &item : text)
    {
        textArray.add(item);
    }
    return data;
}

float calculateRMS(uint8_t *buffer, int bufferSize)
{
    float sum = 0;
    int16_t sample;

    for (int i = 0; i < bufferSize; i += 2)
    {

        sample = (buffer[i + 1] << 8) | buffer[i];
        sum += sample * sample;
    }

    sum /= (bufferSize / 2);

    return sqrt(sum);
}
