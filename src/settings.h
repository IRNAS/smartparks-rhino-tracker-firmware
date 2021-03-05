#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "Arduino.h"
#include <STM32L0.h>
#include "lorawan.h"
#include "stm32l0_eeprom.h"

#define EEPROM_DATA_START_SETTINGS 0

extern boolean settings_updated;

/**
 * @brief LoraWAN settings packet setup - port 3
 * 
 * @details Packet variables have the following information contained:
 * system_status_interval -  send interval in minutes, range 1 - 1440, default 60
 * system_functions - enable/disable certain features
 *    bit 0 - 
 *    bit 1 - 
 *    bit 2 - 
 *    bit 3 - accelerometer enabled
 *    bit 4 - light sensor enabled
 *    bit 5 - temperature sensor enabled
 *    bit 6 - humidity sensor enabled
 *    bit 7 - charging sensor enabled
 * lorawan_datarate_adr - lorawan fix reporting datarate, range 0 - 5 (SF7-SF12), upper nibble is adr, lower nibble is datarate
 */
struct settingsData_t{
  uint16_t  system_status_interval;
  uint8_t   system_functions;
  uint8_t   lorawan_datarate_adr;
  uint8_t   system_voltage_interval; // interval in minutes how often the voltage is measured and checked
  uint8_t   system_charge_min;  // charge voltage minimum 2.8V
  uint8_t   system_charge_max;  // charge voltage maximum 4.2v
  uint16_t  system_input_charge_min; // stop charging when input voltage is less then X
  uint8_t   pulse_threshold; // how many pulses must be received to send a packet
  uint8_t   pulse_on_timeout; // how long is the pulse output on after threshold reached
  uint16_t  pulse_min_interval; // how often at maximum can a device send a packet on pulse event
}__attribute__((packed));

union settingsPacket_t{
  settingsData_t data;
  byte bytes[sizeof(settingsData_t)];
};

static const uint8_t settings_packet_port = 3;
extern settingsPacket_t settings_packet;
extern settingsPacket_t settings_packet_downlink;

uint8_t settings_get_packet_port(void);
void settings_init(void);
void settings_from_downlink(void);
boolean settings_send(void);

#endif
