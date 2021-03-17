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
 * lorawan_datarate_adr - lorawan fix reporting datarate, range 0 - 5 (SF7-SF12), upper nibble is adr, lower nibble is datarate
 */
struct settingsData_t{
  uint8_t   lorawan_datarate_adr;
  uint8_t   sd_power_delay; // After which delay we will power on the WiFi SD Card (in seconds)
  uint8_t   sd_power_time; // How long the power to the SD Card will be turned on (in seconds)
  uint16_t  debounce_time; // How long to wait for additional images after the first detection (in seconds)
  uint16_t  max_debounce_time; // Maximum amount of time to wait for new images (in seconds)
  uint16_t  event_interval; // Amount of time to wait after the last trigger (in seconds)
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
