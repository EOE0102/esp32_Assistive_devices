#ifndef MY_I2S_H
#define MY_I2S_H
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "esp_system.h"

enum MicType {
  ADMP441,
  ICS43434,
  M5GO,
  M5STACKFIRE
};

class MY_I2S {
  i2s_bits_per_sample_t BITS_PER_SAMPLE;
public:
  MY_I2S(uint8_t PIN_I2S_BCLK, uint8_t PIN_I2S_LRC, uint8_t PIN_I2S_DIN);
  int Read(char* data, int numData);
  int GetBitPerSample();
  void clear();
};


void i2s_install();
void i2s_setpin();


#endif // _I2S_H
