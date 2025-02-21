#ifndef MY_WIFI_H
#define MY_WIFI_H

#include <Arduino.h>
#include <WiFi.h>          // 包含WiFi库
#include <HTTPClient.h>    // 包含HTTPClient库

// Web服务器和Preferences对象
void connectToWiFi(const char* ssid, const char* password);
String getTimeFromServer();
String checkAndPrintServerTime(const char* ssid, const char* password);




#endif
