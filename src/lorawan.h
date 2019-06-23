#ifndef LORAWAN_PROJECT_H_
#define LORAWAN_PROJECT_H_

#include "Arduino.h"
#include "LoRaWAN.h"
#include <STM32L0.h>
#include "settings.h"
#include "command.h"
#include "status.h"
#include "sensor.h"

extern boolean lorawan_send_successful;

boolean lorawan_init(void);
boolean lorawan_send(uint8_t port, const uint8_t *buffer, size_t size);
boolean lorawan_joined(void);
void lorawan_joinCallback(void);
void lorawan_checkCallback(void);
void lorawan_receiveCallback(void);
void lorawan_doneCallback(void);

#endif