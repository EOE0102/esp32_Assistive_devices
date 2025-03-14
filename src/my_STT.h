#ifndef MY_STT_H
#define MY_STT_H

#include <Arduino.h>
#include "my_WiFi.h"
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>

#include <HTTPClient.h>

#include <base64.h>
#include "Base64_Arturo.h"

String formatDateForURL(String dateString);
String XF_wsUrl(const char *Secret, const char *Key, String request, String host);


#endif
