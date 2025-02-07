#include <Arduino.h>

#include "esp_camera.h"
#include <FS.h>
#include <SD.h>
#include <SPI.h>

#include <base64.h>
#include <ArduinoJson.h>
#include "cJSON.h"
#include <HTTPClient.h>

// my functions
#include "my_take_photos.h"
#include "my_init.h"

#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM

#include "camera_pins.h"


unsigned long lastCaptureTime = 0; // Last shooting time
int imageCount = 1;                // File Counter
bool camera_sign = false;          // Check camera status
bool sd_sign = false;              // Check sd status






// 1. Replace with your network credentials
// const char *ssid = "TP-LINK_0303";
// const char *password = "xcwm5818006";
// const char *ssid = "myssid";
// const char *password = "mypassword";
const char *ssid = "XuHandy";
const char *password = "qwer1234";

// 2. Replace with your OpenAI API key
const char *apiKey = "sk-d51ad5c147ef4d438e42b7f01f3888db";

// Send request to OpenAI API
String inputText = "你好，通义千问！";
String apiUrl = "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions";
char *data_json;
#define bufferLen 50000
String image_base64()
{
  // Take a photo
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Failed to get camera frame buffer");
    return "camera get error";
  }
  else
  {
    // fb->buf转为base64字符串
    String image = base64::encode(fb->buf, fb->len);
    Serial.println("image_base64 encode success");
    // Release image buffer
    esp_camera_fb_return(fb);
    return image;
  }
}


String image_request(String change) // 发送请求，返回的是请求结果代码，该码要填入下一个函数中（change是图片数据（jpg））
{
  memset(data_json, '\0', sizeof(data_json)); // 清空数组
  change.replace("\r\n", "");                 // 移除换行符

  // strcat(data_json, "{\"model\":\"qwen-vl-plus\",\"messages\":[{\"role\":\"user\",\"content\":[");
  // strcat(data_json, "{\"type\":\"image_url\",\"image_url\":{\"url\":\"data:image/jpeg;base64,");
  // strcat(data_json, change.c_str()); // 添加base64编码数据
  // strcat(data_json, "\"}},");
  // strcat(data_json, "{\"type\":\"text\",\"text\":\"场景里面有什么,请给50字的描述\"}]}]}");

  strcat(data_json, "{\"model\":\"qwen-vl-max-latest\",");
  // strcat(data_json, "{\"model\":\"qwen-plus\",");
  strcat(data_json, "\"messages\":[");
    strcat(data_json, "{");
      strcat(data_json, "\"role\":\"user\",");
      strcat(data_json, "\"content\":[");
      strcat(data_json, "{");
        strcat(data_json, "\"type\":\"image_url\",");
        strcat(data_json, "\"image_url\":{\"url\":\"data:image/jpeg;base64,");
        strcat(data_json, change.c_str()); // 添加base64编码数据
        strcat(data_json, "\"}");
      strcat(data_json,"},");
      strcat(data_json, "{\"type\":\"text\",\"text\":\"场景里面有什么，请以“我看见”开始描述\"}");
      strcat(data_json,"]");
    strcat(data_json,"}");
  strcat(data_json,"]}");
  // Serial.println(data_json);
  // delay(2000);
  HTTPClient http_image_request;
  http_image_request.setTimeout(20000);
  http_image_request.begin(apiUrl);
  http_image_request.addHeader("Content-Type", "application/json");
  http_image_request.addHeader("Authorization", String("Bearer ") + String(apiKey));

  int httpCode = http_image_request.POST(data_json);
  if (httpCode == 200)
  {

    String response = http_image_request.getString();
    http_image_request.end();
    DynamicJsonDocument jsonDoc(1024);
    deserializeJson(jsonDoc, response);
    Serial.println(response);
    String id = jsonDoc["choices"][0]["message"]["content"];
    return id;
  }
  else
  {
    Serial.println("http error code: " + String(httpCode));
    http_image_request.end();
    return "http error";
  }
}









void setup() {
  Serial.begin(115200);
  while(!Serial); // When the serial monitor is turned on, the program starts to execute

  camera_config_t config;
  config = my_pin_init();
  camera_sign = my_camera_init(config);
  sd_sign = my_SD_card_init();

}

void loop() {
  // Camera & SD available, start taking pictures
  if(camera_sign && sd_sign){
    // Get the current time
    unsigned long now = millis();
  
    //If it has been more than 1 minute since the last shot, take a picture and save it to the SD card
    if ((now - lastCaptureTime) >= 10000) {
      char filename[32];
      sprintf(filename, "/image%d.jpg", imageCount);
      photo_save(filename);
      Serial.printf("Saved picture: %s\r\n", filename);
      Serial.println("Photos will begin in one minute, please be ready.");
      imageCount++;
      lastCaptureTime = now;
    }
  }
}
