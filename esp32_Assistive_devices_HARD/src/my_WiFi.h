#ifndef MY_WIFI_H
#define MY_WIFI_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include "HTTPClient.h"
// 与AP模式和Web服务器有关的库
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>


// AP模式的SSID和密码
extern const char *ap_ssid;
extern const char *ap_password;
// Web服务器和Preferences对象
extern AsyncWebServer server;
extern Preferences preferences;

extern bool isWebServerStarted;
extern bool isSoftAPStarted;

void initWiFi_direct(const char* ssid, const char* password);
void getTimeFromServer();





void openWeb();
void handleRoot(AsyncWebServerRequest *request);
void handleWifiManagement(AsyncWebServerRequest *request);
// void handleMusicManagement(AsyncWebServerRequest *request);
// void handleLLMManagement(AsyncWebServerRequest *request);
void handleSave(AsyncWebServerRequest *request);
void handleDelete(AsyncWebServerRequest *request);
void handleList(AsyncWebServerRequest *request);
// void handleSaveMusic(AsyncWebServerRequest *request);
// void handleDeleteMusic(AsyncWebServerRequest *request);
// void handleListMusic(AsyncWebServerRequest *request);
// void handleSaveLLM(AsyncWebServerRequest *request);
// void handleListLLM(AsyncWebServerRequest *request);


#endif
