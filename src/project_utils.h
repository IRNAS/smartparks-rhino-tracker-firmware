#ifndef PROJECT_UTILS_H
#define PROJECT_UTILS_H

#include "Arduino.h"
#include "board.h"
#include "status.h"

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max);
uint32_t get_bits(float x, float min, float max, int precision);
#endif