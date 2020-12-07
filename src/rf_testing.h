#ifndef RF_H_
#define RF_H_

#include "Arduino.h"
#include <STM32L0.h>
#include "lorawan.h"
#include "project_utils.h"
#include "board.h"
#include <math.h>
#include "wiring_private.h"

extern boolean rf_send_flag;

/**
 * @brief RF settings packet setup - port 30
 * 
 */
struct rf_settingsData_t{
    uint32_t freq_start;
    uint32_t freq_stop;
    uint32_t samples;
    int16_t power;
    uint16_t time;
    uint16_t type;
}__attribute__((packed));

union rf_settingsPacket_t{
  rf_settingsData_t data;
  byte bytes[sizeof(rf_settingsData_t)];
};

static const uint8_t rf_vswr_port = 30;
static const uint8_t rf_spectral_port = 31;
extern rf_settingsPacket_t rf_settings_packet;

void rf_init(void);
boolean rf_send(void);

#endif
