
#include <Arduino.h>
#include "base64.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include "cJSON.h"
#include <I2S.h>
#include <ArduinoJson.h>

// #define key 4             //端口0
// #define ADC 2             //端口39
// #define led 15            //端口2
const int buttonPin = 1;  // the number of the pushbutton pin
const int ledPin = 21;    // the number of the LED pin
HTTPClient http_client;
// 1. Replace with your network credentials
const char* ssid = "XuHandy";
const char* password = "qwer1234";
hw_timer_t* timer = NULL;

#define data_len 16000
uint16_t adc_data[data_len];  //16000个数据，8K采样率，即2秒，录音时间为2秒，想要实现更长时间的语音识别，就要改这个数组大小
                              //和下面data_json数组的大小，改大一些。

uint8_t adc_start_flag = 0;     //开始标志
uint8_t adc_complete_flag = 0;  //完成标志


char data_json[45000];  //用于储存json格式的数据,大一点,JSON编码后数据字节数变成原来的4/3,所以得计算好,避免出现越界


void IRAM_ATTR onTimer();
void gain_token(void);
void setup() {

  //Serial.begin(921600);
  Serial.begin(115200);
  // pinMode(ADC, ANALOG);
  // pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  // start I2S at 16 kHz with 16-bits per sample
  I2S.setAllPins(-1, 42, 41, -1, -1);
  if (!I2S.begin(PDM_MONO_MODE, 16000, 16)) {
    Serial.println("Failed to initialize I2S!");
    while (1)
      ;  // do nothing
  }
  uint8_t count = 0;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    count++;
    if (count >= 75) {
      Serial.printf("\r\n-- wifi connect fail! --");
      break;
    }
    vTaskDelay(200);
  }
  Serial.printf("\r\n-- wifi connect success! --\r\n");

  // gain_token();

  timer = timerBegin(0, 80, true);    //  80M的时钟 80分频 1M
  timerAlarmWrite(timer, 125, true);  //  1M  计125个数进中断  8K
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmEnable(timer);
  timerStop(timer);  //先暂停
}


uint32_t time1, time2;
void loop() {

  if (Serial.available() > 0)  //按键按下
  {
    if (Serial.read() == '1') {
      Serial.printf("Start recognition\r\n\r\n");
      digitalWrite(ledPin, HIGH);
      adc_start_flag = 1;
      timerStart(timer);

      // time1=micros();
      while (!adc_complete_flag)  //等待采集完成
      {
        ets_delay_us(10);
      }
      // time2=micros()-time1;

      timerStop(timer);
      adc_complete_flag = 0;  //清标志

      digitalWrite(ledPin, LOW);

      // Serial.printf("time:%d\r\n",time2);  //打印花费时间


      memset(data_json, '\0', strlen(data_json));  //将数组清空
      strcat(data_json, "{");
      strcat(data_json, "\"format\":\"pcm\",");
      strcat(data_json, "\"rate\":16000,");                                                                        //采样率    如果采样率改变了，记得修改该值，只有16000、8000两个固定采样率
      strcat(data_json, "\"dev_pid\":1537,");                                                                      //中文普通话
      strcat(data_json, "\"channel\":1,");                                                                         //单声道
      strcat(data_json, "\"cuid\":\"666666\",");                                                                   //识别码    随便打几个字符，但最好唯一
      strcat(data_json, "\"token\":\"25.68fbc050f08bc5947aceaed0562bfdcf.315360000.2055295779.282335-116708097\",");  //token	这里需要修改成自己申请到的token
      strcat(data_json, "\"len\":32000,");                                                                         //数据长度  如果传输的数据长度改变了，记得修改该值，该值是ADC采集的数据字节数，不是base64编码后的长度
      strcat(data_json, "\"speech\":\"");
      strcat(data_json, base64::encode((uint8_t*)adc_data, sizeof(adc_data)).c_str());  //base64编码数据
      strcat(data_json, "\"");
      strcat(data_json, "}");
      // Serial.println(data_json);


      int httpCode;
      http_client.setTimeout(5000);
      http_client.begin("http://vop.baidu.com/server_api");  //https://vop.baidu.com/pro_api
      http_client.addHeader("Content-Type", "application/json");
      httpCode = http_client.POST(data_json);

      if (httpCode == 200) {
        if (httpCode == HTTP_CODE_OK) {
          String response = http_client.getString();
          http_client.end();
          Serial.println(response);
          // Parse JSON response
          DynamicJsonDocument jsonDoc(512);
          deserializeJson(jsonDoc, response);
          String outputText = jsonDoc["result"][0];
          // 访问"result"数组，并获取其第一个元
          // 输出结果
          Serial.println(outputText);

        } else {
          Serial.printf("[HTTP] GET... failed, error: %s\n", http_client.errorToString(httpCode).c_str());
        }
      }
      // while (!digitalRead(buttonPin))
      //   ;
      Serial.printf("Recognition complete\r\n");
    }
  }
  vTaskDelay(1);
}


uint32_t num = 0;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
void IRAM_ATTR onTimer() {
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  if (adc_start_flag == 1) {
    //Serial.println("");
    // adc_data[num] = analogRead(ADC);
    adc_data[num] = I2S.read();
    num++;
    if (num >= data_len) {
      adc_complete_flag = 1;
      adc_start_flag = 0;
      num = 0;
      //Serial.println(Complete_flag);
    }
  }
  portEXIT_CRITICAL_ISR(&timerMux);
}



// void gain_token(void)  //获取token
// {
//   int httpCode;
//   //注意，要把下面网址中的your_apikey和your_secretkey替换成自己的API Key和Secret Key
//   http_client.begin("https://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id=your_apikey&client_secret=your_secretkey");
//   httpCode = http_client.GET();
//   if (httpCode > 0) {
//     if (httpCode == HTTP_CODE_OK) {
//       String payload = http_client.getString();
//       Serial.println(payload);
//     }
//   } else {
//     Serial.printf("[HTTP] GET... failed, error: %s\n", http_client.errorToString(httpCode).c_str());
//   }
//   http_client.end();
// }
