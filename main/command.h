#ifndef COMMAND_H_
#define COMMAND_H_

#include "Arduino.h"
#include <STM32L0.h>
#include "lorawan.h"
#include <EEPROM.h>
#include "settings.h"
#include "gps_tracker.h"

static uint8_t command_packet_port = 99;
uint8_t command_get_packet_port(void);
void command_receive(uint8_t command);
extern boolean settings_send_flag;
extern boolean gps_log_flag;
extern boolean status_dropoff_flag;

#endif