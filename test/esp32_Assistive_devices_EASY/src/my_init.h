#ifndef MY_INIT_H
#define MY_INIT_H
#include "esp_camera.h"

camera_config_t my_pin_init(void);
bool my_camera_init(const camera_config_t &config);
bool my_SD_card_init(void);


#endif