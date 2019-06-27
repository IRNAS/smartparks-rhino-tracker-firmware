#ifndef GPS_TRACKER_H_
#define GPS_TRACKER_H_

#include "Arduino.h"
#include <STM32L0.h>
#include "settings.h"
#include "command.h"
#include "status.h"
#include "sensor.h"

// Flags for signalling to the rest of the system
extern boolean gps_available_flag;

enum gps_state_e{
  IDLE,
  GENERAL_INIT,
  PERIODIC_INIT,
  TRIGGERED_INIT,
  SLEEP,
  WAKE_UP,
  START,
  RUN,
  STOP,
  SEND,
  DISABLED
};

gps_state_e state = INIT;
gps_state_e state_goto_timeout;
gps_state_e state_prev = INIT;
unsigned long state_timeout_start;
unsigned long state_timeout_duration;

boolean gps_triggered;


#endif