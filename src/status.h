#ifndef STATUS_H_
#define STATUS_H_

#include "Arduino.h"
#include <STM32L0.h>
#include "lorawan.h"
#include "project_utils.h"
#include "settings.h"
#include "board.h"

extern boolean status_send_flag;

/**
 * @brief LoraWAN status packet setup - port 2
 * 
 * @details Packet reporting system staus
 * resetCause - details source of last reset, see decoder.js for meaning
 * mode - what operaiton mode is the device in
 * battery - battery status in %
 * temperature - temperature
 * vbus - voltage
 */
struct statusData_t{
  uint8_t resetCause; // reset cause lower 3 bits, state timeout upper 5 bits
  uint8_t battery;
  uint8_t temperature;
  uint8_t system_functions_errors;
  uint16_t input_voltage;
  uint32_t duration_of_pulse;   
  uint64_t device_id;   
  uint8_t none;                 // Used to align struct to 16 bit (32?) boundary
}__attribute__((packed));

union statusPacket_t{
  statusData_t data;
  byte bytes[sizeof(statusData_t)];
};

static const uint8_t status_packet_port = 12;
extern statusPacket_t status_packet;

void status_scheduler(void);
void status_init(void);
boolean status_send(void);

#endif
