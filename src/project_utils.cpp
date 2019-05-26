#include "project_utils.h"

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max){
 return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//this function needs to be unit tested!
uint32_t get_bits(float x, float min, float max, int precision){
  int range = max - min;
  if (x <= min) {
    x = 0;
  }
  else if (x >= max) {
    x = max - min;
  }
  else {
    x -= min;
  }
  double new_range = (pow(2, precision) - 1) / range;
  uint32_t out_x = x * new_range;
  return out_x;
}