/*
  ESP32_OV5640_AF.h - Library for OV5640 Auto Focus (ESP32 Camera)
  Created by Eric Nam, December 08, 2021.
  Released into the public domain.
*/

#ifndef ESP32_OV5640_AF_h
#define ESP32_OV5640_AF_h

//#include <Arduino.h>
#include "ESP32_OV5640_cfg.h"
#include "esp_camera.h"


bool OV5640_start(sensor_t* _sensor);
uint8_t OV5640_focusInit();
uint8_t OV5640_autoFocusMode();
uint8_t OV5640_getFWStatus();


#endif

