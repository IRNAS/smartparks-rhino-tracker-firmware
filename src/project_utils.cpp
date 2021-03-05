#include "project_utils.h"


/**
 * @brief Maps the variable with the given minimum and maximum value to the value with specified min, max
 * 
 * @param x - value
 * @param in_min - minimum of the value expected
 * @param in_max - maximum of the value expected
 * @param out_min - minimum value of the output
 * @param out_max - maximum value of the output
 * @return float
 */
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max){
 return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/**
 * @brief Maps a specified value to a number with specified precision in number of bits between the given min and max
 * 
 * @param x 
 * @param min 
 * @param max 
 * @param precision 
 * @return uint32_t 
 */
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