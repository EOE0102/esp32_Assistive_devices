#include <Arduino.h>
#include "base64.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include "cJSON.h"
#include <ArduinoJson.h>
#include "Audio.h"
// #按键和max9814麦克风
#define key 3
#define ADC 2
//max98357扬声器引脚
#define I2S_DOUT 6  // DIN connection
#define I2S_BCLK 5  // Bit clock
#define I2S_LRC 4   // Left Right Clock
Audio audio;
HTTPClient http,http1,http2;
String voice_id = "female-tianmei-jingpin";  //青年大学生音色：male-qn-daxuesheng;甜美女性音色：female-tianmei;男性主持人：presenter_male;女性主持人：presenter_female
// 1. Replace with your network credentials
const char *ssid = "XuHandy";
const char *password = "qwer1234";
// 2. Check your Aduio port
const int ledPin = 21;  // the number of the LED pin
hw_timer_t *timer = NULL;
const int adc_data_len = 16000 * 3;
const int data_json_len = adc_data_len * 2 * 1.4;
uint16_t *adc_data;
char *data_json;
uint8_t adc_start_flag = 0;     //开始标志
uint8_t adc_complete_flag = 0;  //完成标志


// 3. Replace with your MiniMax API key
const char *mini_apiKey = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJHcm91cE5hbWUiOiIyMzQ1dm9yIiwiVXNlck5hWfIf40Z_X8Aog3_bUAqsqZZmbb7ig7zTDAfoAWjmeUHdQ1TX16rg6dV5aPuJj02yU6gjROQZZYUPk3lUqdHV2WLUvQcZFlerkW0F9AtkfLblDCyEEHvQLG5dbcPCTGiRWLt3I3AZ_l1YSWmPJkazrQPUN5BVlb5AHjIVKwWyETDd63eydY7DmpqOSGJYjc4sRJFMCccpOjTa66iLBhfBsrkfZsg";
const char *tts_url = "https://api.minimax.chat/v1/t2a_pro?GroupId=17595904";
const char *chat_url = "https://api.minimax.chat/v1/text/chatcompletion_v2";
const char *stt_url = "http://vop.baidu.com/server_api";
// 4. Replace with your baidu voice detect token
String baidu_token = "24.d977d47af91cdbb34fee2b2282335-57722200";
String mini_token_key = String("Bearer ") + mini_apiKey;
// Send request to MiniMax API
String inputText = "你好，minimax！";

String response, question, aduiourl;
String answer = "我是小西，你好鸭";
DynamicJsonDocument jsonDoc(1024);
uint32_t num = 0;
uint32_t time1, time2;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
// 函数声明
void IRAM_ATTR onTimer();
String sendToSTT();
String getGPTAnswer(String inputText);
String getvAnswer(String ouputText);

void setup() {
  Serial.begin(115200);
  adc_data = (uint16_t *)ps_malloc(adc_data_len * sizeof(uint16_t));  //ps_malloc 指使用片外PSRAM内存
  if (!adc_data) {
    Serial.println("Failed to allocate memory for adc_data");
  }
  data_json = (char *)ps_malloc(data_json_len * sizeof(char));  // 根据需要调整大小
  if (!data_json) {
    Serial.println("Failed to allocate memory for data_json");
  }
  pinMode(ADC, ANALOG);
  pinMode(key, INPUT_PULLUP);
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(18);  // 0...21
  pinMode(ledPin, OUTPUT);
  uint8_t count = 0;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    count++;
    if (count >= 75) {
      Serial.printf("\r\n-- wifi connect fail! --");
      ESP.restart();  // 执行软件重启
    }
    vTaskDelay(200);
  }
  Serial.printf("\r\n-- wifi connect success! --\r\n");
  http.setTimeout(5000);
  // gain_token();
  //设置定时器16k采样音频
  timer = timerBegin(0, 40, true);    //  80M的时钟 40分频 2M
  timerAlarmWrite(timer, 125, true);  //  2M  计125个数进中断  16K
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmEnable(timer);
  timerStop(timer);  //先暂停
}

void loop() {
  audio.loop();
  if (digitalRead(key) == 1) {
    delay(10);
    if (digitalRead(key) == 1) {
      Serial.printf("Start recognition\r\n");
      digitalWrite(ledPin, LOW);
      adc_start_flag = 1;
      timerStart(timer);
      while (!adc_complete_flag)  //等待采集完成
      {
        ets_delay_us(10);
      }
      timerStop(timer);
      adc_complete_flag = 0;  //清标志
      digitalWrite(ledPin, HIGH);
      question = sendToSTT();
      if (question != "error") {
        Serial.println("Input:" + question);
        answer = getGPTAnswer(question);
        if (answer != "error") {
          Serial.println("Answer: " + answer);
          aduiourl = getvAnswer(answer);
          if (aduiourl != "error") {
            audio.stopSong();
            audio.connecttohost(aduiourl.c_str());  //  128k mp3
          }
        }
      }
      Serial.println("Recognition complete\r\n");
    }
  }
  while (Serial.available() > 0) {
    char voice = Serial.read();
    // Serial.println(voice);
    switch (voice) {
      case '1':
        voice_id = "male-qn-daxuesheng";
        break;
      case '2':
        voice_id = "female-tianmei";
        break;
      case '3':
        voice_id = "presenter_male";
        break;
      case '4':
        voice_id = "presenter_female";
        break;
       case '5':
       //5.replace your clone voice_id
        voice_id = "vor_test";
        break;
    }
    Serial.println(voice_id);
  }
  vTaskDelay(5);
}
//录音函数
void IRAM_ATTR onTimer() {
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  if (adc_start_flag == 1) {
    //Serial.println("");
    adc_data[num] = analogRead(ADC);
    num++;
    if (num >= adc_data_len) {
      adc_complete_flag = 1;
      adc_start_flag = 0;
      num = 0;
      //Serial.println(Complete_flag);
    }
  }
  portEXIT_CRITICAL_ISR(&timerMux);
}

//stt语音识别
String sendToSTT() {
  memset(data_json, '\0', data_json_len * sizeof(char));
  strcat(data_json, "{");
  strcat(data_json, "\"format\":\"pcm\",");
  strcat(data_json, "\"rate\":16000,");
  strcat(data_json, "\"dev_pid\":1537,");
  strcat(data_json, "\"channel\":1,");
  strcat(data_json, "\"cuid\":\"57722200\",");
  strcat(data_json, "\"token\":\"");
  strcat(data_json, baidu_token.c_str());
  strcat(data_json, "\",");
  sprintf(data_json + strlen(data_json), "\"len\":%d,", adc_data_len * 2);
  strcat(data_json, "\"speech\":\"");
  strcat(data_json, base64::encode((uint8_t *)adc_data, adc_data_len * sizeof(uint16_t)).c_str());
  strcat(data_json, "\"");
  strcat(data_json, "}");
  http.begin(stt_url);  //https://vop.baidu.com/pro_api
  http.addHeader("Content-Type", "application/json");
  // Serial.print(data_json);
  int httpResponseCode = http.POST(data_json);
  if (httpResponseCode == 200) {
    response = http.getString();
    http.end();
    Serial.print(response);
    deserializeJson(jsonDoc, response);
    String question = jsonDoc["result"][0];
    // 访问"result"数组，并获取其第一个元
    return question;
  } else {
    http.end();
    Serial.printf("stt_error: %s\n", http.errorToString(httpResponseCode).c_str());
    return "error";
  }
}
//chatgpt对话
String getGPTAnswer(String inputText) {
  http1.begin(chat_url);
  http1.addHeader("Content-Type", "application/json");
  http1.addHeader("Authorization", mini_token_key);
  String payload = "{\"model\":\"abab5.5s-chat\",\"messages\":[{\"role\": \"system\",\"content\": \"你是鹏鹏的生活助手机器人，要求下面的回答严格控制在256字符以内。\"},{\"role\": \"user\",\"content\": \"" + inputText + "\"}]}";
  int httpResponseCode = http1.POST(payload);
  if (httpResponseCode == 200) {
    response = http1.getString();
    http1.end();
    Serial.println(response);
    deserializeJson(jsonDoc, response);
    String answer = jsonDoc["choices"][0]["message"]["content"];
    return answer;
  } else {
    // http1.end();
    response = http1.getString();
    http1.end();
    Serial.println(response);
    Serial.printf("chatError %i \n", httpResponseCode);
    return "error";
  }
}
//tts语音播报
String getvAnswer(String ouputText) {
  http2.begin(tts_url);
  http2.addHeader("Content-Type", "application/json");
  http2.addHeader("Authorization", mini_token_key);
  // 创建一个StaticJsonDocument对象，足够大以存储JSON数据
  StaticJsonDocument<200> doc;
  // 填充数据
  doc["text"] = ouputText;
  doc["model"] = "speech-01";
  doc["audio_sample_rate"] = 32000;
  doc["bitrate"] = 128000;
  doc["voice_id"] = voice_id;
  // 创建一个String对象来存储序列化后的JSON字符串
  String jsonString;
  // 序列化JSON到String对象
  serializeJson(doc, jsonString);
  int httpResponseCode = http2.POST(jsonString);
  if (httpResponseCode == 200) {
    response = http2.getString();
    Serial.println(response);
    http2.end();
    deserializeJson(jsonDoc, response);
    String aduiourl = jsonDoc["audio_file"];
    return aduiourl;
  } else {
    Serial.printf("tts %i \n", httpResponseCode);
    http2.end();
    return "error";
  }
}
// optional
void audio_info(const char *info) {
  Serial.print("info        ");
  Serial.println(info);
}
void audio_eof_mp3(const char *info) {  //end of file
  Serial.print("eof_mp3     ");
  Serial.println(info);
}
