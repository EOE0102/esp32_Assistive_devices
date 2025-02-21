#include <Arduino.h>

#include "esp_camera.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <base64.h>
// #include "MY_WIFI.h"
#include <ArduinoJson.h>
#include "cJSON.h"
#include <HTTPClient.h>
#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 10
#define SIOD_GPIO_NUM 40
#define SIOC_GPIO_NUM 39

#define Y9_GPIO_NUM 48
#define Y8_GPIO_NUM 11
#define Y7_GPIO_NUM 12
#define Y6_GPIO_NUM 14
#define Y5_GPIO_NUM 16
#define Y4_GPIO_NUM 18
#define Y3_GPIO_NUM 17
#define Y2_GPIO_NUM 15
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM 47
#define PCLK_GPIO_NUM 13

#define LED_GPIO_NUM 21

// 1. Replace with your network credentials
const char *ssid = "XuHandy";
const char *password = "qwer1234";
unsigned long lastCaptureTime = 0; // Last shooting time
int imageCount = 1;                // File Counter
bool camera_sign = false;          // Check camera status
bool sd_sign = false;              // Check sd status

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
    return "error";
  }
  else
  {
    // fb->buf转为base64字符串
    String image = base64::encode(fb->buf, fb->len);
    Serial.println("image_base64 success");
    // Release image buffer
    esp_camera_fb_return(fb);
    Serial.println("esp_camera_fb_return");
    return image;
  }
}

String image_request(String change) // 发送请求，返回的是请求结果代码，该码要填入下一个函数中（change是图片数据（jpg）
{
  memset(data_json, '\0', sizeof(data_json)); // 清空数组
  Serial.println("memset");
  change.replace("\r\n", "");                 // 移除换行符
  Serial.println("replace");
  strcat(data_json, "{\"model\":\"qwen-vl-plus\",\"messages\":[");
  strcat(data_json, "{\"role\":\"user\",\"content\":[{\"type\":\"text\",\"text\":\"You are a helpful assistant.\"}]},");
  strcat(data_json, "{\"role\":\"user\",\"content\":[");
  strcat(data_json, "{\"type\":\"image_url\",\"image_url\":{\"url\":\"data:image/jpeg;base64,");
  strcat(data_json, change.c_str()); // 添加base64编码数据
  strcat(data_json, "\"}},");
  strcat(data_json, "{\"type\":\"text\",\"text\":\"这是什么\"}]}]}");

  // String data_json = String("{\"model\":\"qwen-vl-max-latest\",\"messages\":[{\"role\":\"user\",\"content\":[{\"type\":\"image_url\",\"image_url\":{\"url\":\"data:image/jpeg;base64,") + 
  //   change + 
  //   String("\"}},{\"type\":\"text\",\"text\":\"场景里面有什么，请以“我看见”开始描述\"}]}]}");
  // Serial.println(data_json);
  Serial.println("data_json");
  delay(2000);
  HTTPClient http_image_request;
  Serial.println("HTTPClient");
  http_image_request.setTimeout(20000);
  http_image_request.begin(apiUrl);
  Serial.println("begin(apiUrl)");
  http_image_request.addHeader("Content-Type", "application/json");
  http_image_request.addHeader("Authorization", String("Bearer ") + String(apiKey));
  Serial.println("addHeader");
  int httpCode = http_image_request.POST(data_json);
  Serial.println("http_image_request");
  if (httpCode == 200)
  {
    // 增加 HTTPClient 的缓冲区大小
    String response = http_image_request.getString();
    http_image_request.end();
    DynamicJsonDocument jsonDoc(1024);
    deserializeJson(jsonDoc, response);
    Serial.println("HTTP response: " +response);
    String id = jsonDoc["choices"][0]["message"]["content"];
    return id;
  }
  else
  {
    Serial.println("error request:" + String(httpCode));
    http_image_request.end();
    return "error";
  }
}

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ; // When the serial monitor is turned on, the program starts to execute
  // Connect to Wi-Fi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  data_json = (char *)ps_malloc(bufferLen * sizeof(char)); // 根据需要调整大小
  if (!data_json)
  {
    Serial.println("Failed to allocate memory for data_json");
  }
  Serial.println("Starting Camera");

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG)
  {
    if (psramFound())
    {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      // config.grab_mode = CAMERA_GRAB_LATEST;
    }
    else
    {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  }
  else
  {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  Serial.println("Camera Ready");

  camera_sign = true; // Camera initialization check passes
}



void loop()
{
  // Camera & SD available, start taking pictures
  if (Serial.available())
  {
    if (Serial.read() == '1')
    {
      // Get the current time
      unsigned long now = millis();

      // If it has been more than 1 minute since the last shot, take a picture and save it to the SD card
      if ((now - lastCaptureTime) >= 1500)
      {
        String image = image_base64();
        if (image != "error")
        {
          // Save the image to the SD card
          String image_answer = image_request(image);
          if (image_answer != "error")
          {
            Serial.println("Image sent to OpenAI API, answer: " + image_answer);
          }
        }
        lastCaptureTime = now;
      }
    }
  }
}
