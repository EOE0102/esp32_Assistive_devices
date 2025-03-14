#ifndef AUDIO_RECORD_H
#define AUDIO_RECORD_H

#include <Arduino.h>
#include "base64.h"
#include "MY_I2S.h"
#include <WiFi.h>
#include <ArduinoJson.h>

// RecordAndSaveFile()
#include <FS.h>
#include <SD.h>
#include <SPI.h>

// 16位，单声道，16000Hz，线性PCM
class Audio_Record
{
  MY_I2S *i2s;
  static const int headerSize = 44;
  static const int i2sBufferSize = 5120;
  char i2sBuffer[i2sBufferSize];
  const int DATA_JSON_SIZE = 512;
  char data_json[512];
  String Question = "";

  WiFiClient client; // @suppress("Abstract class cannot be instantiated")
  // WiFiClientSecure clientsecure; // @suppress("Abstract class cannot be instantiated")
  WiFiClient *_client = nullptr;
  const char *header_format = "{"
                              "\"format\":\"pcm\","
                              "\"rate\":16000,"
                              "\"dev_pid\":1537,"
                              "\"channel\":1,"
                              "\"cuid\":\"\","
                              "\"token\":\"\","
                              "\"speech\":\"";

  const char *footer_format = "\",\"len\":";
  void CreateWavHeader(byte *header, int waveDataSize);
  String parseJSON(const char *jsonResponse);

public:
  static const int wavDataSize = 30000; // 必须是dividedWavDataSize的倍数。录音时间约为1.9秒。
  static const int dividedWavDataSize = i2sBufferSize / 4;
  char **wavData;                          // 分段存储。因为在ESP32中无法分配大块连续内存区域。
  byte paddedHeader[headerSize + 4] = {0}; // 大小必须是3的倍数以便进行Base64编码。额外的字节大小必须是偶数，因为音频数据是16位的。

  Audio_Record(uint8_t PIN_I2S_BCLK, uint8_t PIN_I2S_LRC, uint8_t PIN_I2S_DIN);
  ~Audio_Record();
  void init();
  void Record();
  void SaveFile();//For Debug
  float calculateRMS(uint8_t *buffer, int bufferSize);
  void clear();

};

#endif // _AUDIO_H
