#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "Arduino.h"
#include "TimerMillis.h"
#include <STM32L0.h>
#include "lorawan.h"

extern boolean settings_send_flag;
extern boolean settings_updated;

/**
 * @brief LoraWAN settings packet setup - port 3
 * 
 * @details Packet variables have the following information contained:
 * system_status_interval -  send interval in minutes, range 1 - 1440, default 60
 * system_functions - enable/disable certain features
 *    bit 0 - gps periodic enabled
 *    bit 1 - gps triggered enabled
 *    bit 2 - gps cold fix = 0 / hot fix = 1
 *    bit 3 - accelerometer enabled
 *    bit 4 - light sensor enabled
 *    bit 5 - temperature sensor enabled
 *    bit 6 - humidity sensor enabled
 *    bit 7 - pressure sensor enabled
 * lorawan_datarate_adr - lorawan fix reporting datarate, range 0 - 5 (SF7-SF12), upper nibble is adr, lower nibble is datarate
 * sensor_interval -  send interval in minutes, range 1 - 1440, default 60
 * gps_cold_fix_timeout - cold fix timeout in seconds, range 0-600
 * gps_hot_fix_timeout - hot fix timeout in seconds, range 0-600
 * gps_minimal_ehpe - minimal hdop to have a valid fix- values are *10, thus divide to get the number
 * mode_slow_voltage_threshold - in % of the charge, range 1 - 100
 * gps_settings - enable/disable certain features
 *    bit 0 - gps 3d fix required
 */
struct settingsData_t{
  uint16_t  system_status_interval;
  uint8_t   system_functions;
  uint8_t   lorawan_datarate_adr;
  uint16_t  sensor_interval;
  uint16_t  gps_cold_fix_timeout;
  uint16_t  gps_hot_fix_timeout;
  uint8_t   gps_minimal_ehpe;
  uint8_t   mode_slow_voltage_threshold;
  uint8_t   gps_settings;
}__attribute__((packed));

union settingsPacket_t{
  settingsData_t data;
  byte bytes[sizeof(settingsData_t)];
};

static const uint8_t settings_packet_port = 3;
extern settingsPacket_t settings_packet;
extern settingsPacket_t settings_packet_downlink;

uint8_t settings_get_packet_port(void);
void settings_timer_callback(void);
void settings_init(void);
void settings_from_downlink(void);
boolean settings_send(void);

#endif