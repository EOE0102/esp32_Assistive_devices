#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// 1. Replace with your network credentials
const char* ssid = "XuHandy";
const char* password = "qwer1234";

// 2. Replace with your OpenAI API key
const char* apiKey = "sk-5554bc3278ef431e9099f5e05dbf6221";

// Send request to OpenAI API
String inputText = "你好，通义千问！";
String apiUrl = "https://dashscope.aliyuncs.com/api/v1/services/aigc/text-generation/generation";

String answer;
String getGPTAnswer(String inputText) {
  HTTPClient http;
  http.setTimeout(10000);
  http.begin(apiUrl);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String(apiKey));
  String payload = "{\"model\":\"qwen-turbo\",\"input\":{\"messages\":[{\"role\": \"system\",\"content\": \"你是鹏鹏的生活助手机器人，要求下面的回答严格控制在256字符以内。\"},{\"role\": \"user\",\"content\": \"" + inputText + "\"}]}}";
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

void setup() {
  // Initialize Serial
  Serial.begin(115200);

  // Connect to Wi-Fi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  answer = getGPTAnswer(inputText);
  Serial.println("Answer: " + answer);
  Serial.println("Enter a prompt:");

}

void loop() {
  // do nothing
  if (Serial.available()) {
    inputText = Serial.readStringUntil('\n');
    // inputText.trim();
    Serial.println("\n Input:"+inputText);

    answer = getGPTAnswer(inputText);
    Serial.println("Answer: " + answer);
    Serial.println("Enter a prompt:");
  }
  // delay(2);
}


