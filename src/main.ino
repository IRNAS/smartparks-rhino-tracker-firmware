#include <STM32L0.h> 
#include "TimerMillis.h"
#include "command.h"
#include "lorawan.h"
#include "sensor.h"
#include "settings.h"
#include "status.h"
#include "project_utils.h"

//#define debug
//#define serial_debug  Serial

// Initialize timer for watchdog
TimerMillis watchdog;

/*
 *  Function:     void callbackWatchdog(void)
 *  Description:  function called with timer to kick the watchdog
 */

void callbackWatchdog(void)
{
  STM32L0.wdtReset();
  
  /*#ifdef debug
    serial_debug.print("callbackWatchdog(): ");
    serial_debug.println(millis());
  #endif*/

  // if any of the flags are high, wake up
  if(settings_updated|settings_send_flag|status_send_flag|sensor_send_flag){
    STM32L0.wakeup();
    #ifdef debug
      serial_debug.print("callbackWatchdog(): wakeup: ");
      serial_debug.println(millis());
    #endif
  }
  
}

void setup() {
  // Watchdog
  STM32L0.wdtEnable(18000);
  watchdog.start(callbackWatchdog, 0, 8500);

  // Serial port debug setup
  #ifdef debug
    serial_debug.begin(115200);
    serial_debug.println("setup(): serial debug begin");
  #endif

  // setup lorawan
  settings_init();
  lorawan_init();
  status_init();
  sensor_init();
  //send all messages out on boot
  settings_send_flag = true;
  status_send_flag = true;
  sensor_send_flag = true;
}


void loop() {
  //event processing loop
  //check all the flags and handle them in priority order
  //note one per loop is processed, ensuring minimum 30s lorawan packet spacing
  //check if settings have been updated

  #ifdef debug
    serial_debug.print("loop( ");
    serial_debug.print("settings_updated: ");
    serial_debug.print(settings_updated);
    serial_debug.print(" settings_send_flag: ");
    serial_debug.print(settings_send_flag);
    serial_debug.print(" sensor_send_flag: ");
    serial_debug.print(sensor_send_flag);
    serial_debug.print(" status_send_flag: ");
    serial_debug.print(status_send_flag);
    serial_debug.println(" )");
  #endif

  
  if(settings_updated==true){
    status_init();
    sensor_init();
    settings_updated=false;
    settings_send_flag = true;
    status_send_flag = true;
    sensor_send_flag = true;
  }
  if(settings_send_flag==true){
    settings_send();
    settings_send_flag=false;
  }
  else if(sensor_send_flag==true){
    sensor_send();
    sensor_send_flag=false;
  }
  else if(status_send_flag==true){
    status_send();
    status_send_flag=false;
  }
  //go to stop state
  STM32L0.stop();
}