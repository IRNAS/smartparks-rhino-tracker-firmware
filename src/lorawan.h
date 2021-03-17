#ifndef LORAWAN_PROJECT_H_
#define LORAWAN_PROJECT_H_

#include "Arduino.h"
#include <LoRaWAN.h>
#include <STM32L0.h>
#include "settings.h"
#include "command.h"
#include <TimerMillis.h>
#include "board.h"
#include "status.h"
#include "stm32l0_eeprom.h"

extern boolean lorawan_send_successful;

boolean lorawan_init(void);
boolean lorawan_send(uint8_t port, const uint8_t *buffer, size_t size);
boolean lorawan_joined(void);
void lorawan_joinCallback(void);
void lorawan_checkCallback(void);
void lorawan_receiveCallback(void);
void lorawan_doneCallback(void);

#if defined(DATA_EEPROM_BANK2_END)
#define EEPROM_OFFSET_START            ((((DATA_EEPROM_BANK2_END - DATA_EEPROM_BASE) + 1023) & ~1023) - 1024)
#else
#define EEPROM_OFFSET_START            ((((DATA_EEPROM_END - DATA_EEPROM_BASE) + 1023) & ~1023) - 1024)
#endif

#define EEPROM_OFFSET_COMMISSIONING    (EEPROM_OFFSET_START + 0)

#endif