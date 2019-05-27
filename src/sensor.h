#ifndef SENSOR_H_
#define SENSOR_H_

#include "Arduino.h"
#include "TimerMillis.h"
#include <STM32L0.h>
#include "lorawan.h"
#include "settings.h"
#include "status.h"

extern boolean sensor_send_flag;

/**
 * @brief LoraWAN sensor packet setup - port 1
 * 
 */
struct sensorData_t{
  uint32_t lat;
  uint32_t lon;
  uint16_t alt;
  uint8_t hdop;
  uint8_t time_to_fix;
}__attribute__((packed));

union sensorPacket_t{
  sensorData_t data;
  byte bytes[sizeof(sensorData_t)];
};

static const uint8_t sensor_packet_port = 1;
extern sensorPacket_t sensor_packet;

void sensor_timer_callback(void);
void sensor_init(void);
void sensor_send(void);

#endif