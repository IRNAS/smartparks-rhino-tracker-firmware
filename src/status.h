#ifndef STATUS_H_
#define STATUS_H_

#include "Arduino.h"
#include "settings.h"
#include <STM32L0.h>
#include "lorawan.h"
#include "project_utils.h"
#include "settings.h"
#include "board.h"
#include "LIS2DW12.h"
#include <Adafruit_ADS1015.h>


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
 * system_functions_errors - errors of different modules, only applicable if respective module is enabled
 *    bit 0 - gps periodic error
 *    bit 1 - gps triggered error
 *    bit 2 - gps fix error
 *    bit 3 - accelerometer error
 *    bit 4 - light sensor error
 *    bit 5-7 - charging
 */
struct statusData_t{
  uint8_t resetCause; // reset cause lower 3 bits, state timeout upper 5 bits
  uint8_t battery;
  uint8_t temperature;
  uint8_t system_functions_errors;
  uint8_t lat1;
  uint8_t lat2;
  uint8_t lat3;
  uint8_t lon1;
  uint8_t lon2;
  uint8_t lon3;
  uint8_t gps_resend;
  uint8_t accelx;
  uint8_t accely;
  uint8_t accelz;
  uint16_t input_voltage;
  uint16_t gps_on_time_total;
  uint32_t gps_time;
  uint8_t pulse_count;
  uint8_t pulse_energy;
  uint16_t pulse_voltage;
  uint8_t ctzn_temp;
  uint16_t conductivity_ctz;
  uint16_t salinity;
  uint16_t conductivity_no_comp;
  uint8_t optod_temp;
  uint16_t oxygen_sat;
  uint16_t oxygen_mgL;
  uint16_t oxygen_ppm;
  uint8_t bme_temp;
  uint8_t bme_pressure;
  uint8_t bme_humid;
}__attribute__((packed));

union statusPacket_t{
  statusData_t data;
  byte bytes[sizeof(statusData_t)];
};

static const uint8_t status_packet_port = 12;
extern statusPacket_t status_packet;

void status_scheduler(void);
void status_init(void);
void status_measure_voltage(void);
boolean status_send(void);
void status_accelerometer_init(void);
accel_data status_accelerometer_read(void);
void status_fence_monitor_calibrate(uint16_t calibrate_value);
void status_fence_monitor_read();
void status_dropoff();

#endif
