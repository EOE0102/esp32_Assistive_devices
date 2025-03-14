#include <WiFi.h>
#include <HTTPClient.h>
// #include <UrlEncode.h>
#include "Arduino.h"
#include "WiFiMulti.h"
#include "Audio.h"

// 1、修改MAX98357喇叭接口
#define I2S_LRC D4 // LR Clock
#define I2S_BCLK D5   // Bit Clock
#define I2S_DOUT D6  // Data Out

Audio audio;
WiFiMulti wifiMulti;
// 2、修改WiFi密码
const char *ssid = "XuHandy";
const char *password = "qwer1234";  // Change this to your WiFi password
// 3、修改百度语音助手的用户信息
const char *API_KEY = "BXL2YS5w67Xw5XDq";
const char *SECRET_KEY = "pb2zIW2Nch2uNtceKX";
// 4、修改播放文本内容
String encodedText = "人家刚满18岁";

const int PER = 4;
const int SPD = 5;
const int PIT = 5;
const int VOL = 5;
const int AUE = 6;

const char *TTS_URL = "https://tsn.baidu.com/text2audio";
String url = TTS_URL;

//********** 百度 API 配置 **********
const char *client_id = "IwWjf6XvMgCSB2uE8zbIvuHM";
const char *client_secret = "Uj9LmuRDqhMtAWktR0L4CTRTgODSsW3r";
String access_token = "";


//********** URL 编码函数 **********
String urlEncode(String str)
{
  String encodedString = "";
  char c;
  char buf[4];
  for (int i = 0; i < str.length(); i++)
  {
    c = str.charAt(i);
    if (isalnum(c))
    {
      encodedString += c;
    }
    else
    {
      sprintf(buf, "%%%02X", c);
      encodedString += buf;
    }
  }
  return encodedString;
}



//********** 获取百度 Access Token **********
String getAccessToken()
{
  HTTPClient http;
  String url = "https://aip.baidubce.com/oauth/2.0/token"
               "?grant_type=client_credentials"
               "&client_id=" +
               String(client_id) +
               "&client_secret=" + String(client_secret);

  http.begin(url);
  int httpResponseCode = http.GET();
  String token = "";
  if (httpResponseCode == HTTP_CODE_OK)
  {
    String response = http.getString();
    // 简单解析 access_token 字符串（实际建议使用 JSON 库进行解析）
    int index = response.indexOf("access_token\":\"");
    if (index != -1)
    {
      token = response.substring(index + 15);
      token = token.substring(0, token.indexOf("\""));
    }
  }
  else
  {
    Serial.print("获取 access_token 失败，HTTP 状态码: ");
    Serial.println(httpResponseCode);
  }
  http.end();
  return token;
}





void tts_get() {
  const char *headerKeys[] = { "Content-Type", "Content-Length" };
  // 5、修改百度语音助手的token
  url += "?tok=" + access_token;
  url += "&tex=" + encodedText;
  url += "&per=" + String(PER);
  url += "&spd=" + String(SPD);
  url += "&pit=" + String(PIT);
  url += "&vol=" + String(VOL);
  url += "&aue=" + String(AUE);
  url += "&cuid=esp32s3";
  url += "&lan=zh";
  url += "&ctp=1";

  HTTPClient http;

  Serial.print("URL: ");
  Serial.println(url);

  http.begin(url);
  http.collectHeaders(headerKeys, 2);
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    if (httpResponseCode == HTTP_CODE_OK) {
      Serial.print("Content-Type = ");
      Serial.println(http.header("Content-Type"));
      String contentType = http.header("Content-Type");
      if (contentType.startsWith("audio")) {
        Serial.println("合成成功，返回的是音频文件");
        // 处理音频文件，保存到SD卡或者播放
      } else if (contentType.equals("application/json")) {
        Serial.println("合成出现错误，返回的是JSON文本");
        // 处理错误信息，根据需要进行相应的处理
      } else {
        Serial.println("未知的Content-Type");
        // 可以添加相应的处理逻辑
      }
    } else {
      Serial.println("Failed to receive audio file");
    }
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}



void player() {
  // WiFi.mode(WIFI_STA);
  // wifiMulti.addAP(ssid.c_str(), password.c_str());
  // wifiMulti.run();
  // if(WiFi.status() != WL_CONNECTED){
  //     WiFi.disconnect(true);
  //     wifiMulti.run();
  // }
  const char *host = url.c_str();
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(12);        // 0...21
  audio.connecttohost(host);  //  128k mp3
}






void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  encodedText = urlEncode(urlEncode(encodedText));
  tts_get();
  player();
}



void loop() {
  audio.loop();
  if (Serial.available()) {  // put streamURL in serial monitor
    audio.stopSong();
    String r = Serial.readString();
    r.trim();
    if (r.length() > 5) audio.connecttohost(r.c_str());
    log_i("free heap=%i", ESP.getFreeHeap());
  }
}

