#include <STM32L0.h> 
#include "TimerMillis.h"
#include "board.h"
#include "command.h"
#include "lorawan.h"
#include "sensor.h"
#include "settings.h"
#include "status.h"
#include "project_utils.h"

#define debug
#define serial_debug  Serial

// Initialize timer for periodic callback
TimerMillis periodic;

long event_status_last = 0;
long event_sensor_last = 0;
long event_loop_start = 0;

enum state_e{
  INIT,
  LORAWAN_INIT,
  LORAWAN_JOIN,
  GENERAL_INIT,
  IDLE,
  SETTINGS_SEND,
  STATUS_SEND,
  GPS_START,
  GPS_READ,
  SENSOR_READ,
  SENSOR_SEND,
  LORAWAN_TRANSMIT,
  HIBERNATION
} ;

state_e state = INIT;
state_e state_next;
state_e state_prev;
unsigned long state_timeout_start;
unsigned long state_timeout_duration;

// function prototypes because Arduino failes if using enum otherwise
void callbackPeriodic(void);
void state_transition(state_e next);
bool state_check_timeout(void);


/*
 *  Function:     void callbackPeriodic(void)
 *  Description:  function called with timer to kick the watchdog
 */

void callbackPeriodic(void){
  STM32L0.wdtReset();
  
  /*#ifdef debug
    serial_debug.print("wdt(): ");
    serial_debug.println(millis());
  #endif*/

  unsigned long elapsed;
  // determine which events need to be scheduled

  elapsed = millis()-event_status_last;
  if(elapsed>=(settings_packet.data.system_status_interval*60*1000)){
    event_status_last=millis();
    status_send_flag = true;
  }
  elapsed = millis()-event_sensor_last;
  if(elapsed>=(settings_packet.data.sensor_interval*60*1000)){
    event_sensor_last=millis();
    sensor_send_flag = true;
  }

  // if the main loop is running and not sleeping
  if(event_loop_start!=0){
    elapsed = millis()-event_loop_start;
    // if loop has been running for more then 60s, then reboot system
    if(elapsed>=60*1000){
      STM32L0.reset();
    }
  }

  // if any of the flags are high, wake up
  if(settings_updated|settings_send_flag|status_send_flag|sensor_send_flag){
    STM32L0.wakeup();
    /*#ifdef debug
      serial_debug.print("wakeup(");
      serial_debug.print(millis());
      serial_debug.println(")");
    #endif*/
  }
}

/**
 * @brief change to next state and implement a timeout for each state
 * 
 */
void state_transition(state_e next){
  // mark the time state has been entered
  state_timeout_start = millis();
  // move to the following state
  state_prev=state;
  state=next;
}

/**
 * @brief check if the state has timed out
 * 
 */
bool state_check_timeout(void){
  // timeout can be disabled
  if(state_timeout_duration==0){
    return false;
  }
  unsigned long elapsed = millis()-state_timeout_start;
  //check if we have been in the existing state too long
  if(elapsed >=state_timeout_duration){
    return true;
  }
  return false;
}

void setup() {
  // Watchdog
  STM32L0.wdtEnable(18000);
  periodic.start(callbackPeriodic, 0, 5000);

  // Serial port debug setup
  #ifdef serial_debug
    serial_debug.begin(115200);
  #endif
  #ifdef debug
    serial_debug.println("setup(serial debug begin)");
  #endif

  // start the FSM with LoraWAN init
  state = INIT;
}


void loop() {
  long sleep = -1;
  event_loop_start = millis();
  // FSM implementaiton for clarity of process loop
  switch (state)
  {
  case INIT:
    // defaults for timing out
    state_timeout_duration=0;
    state_next=INIT;
    // setup default settings
    settings_init();
    // load settings, currently can not return an error, thus proceed directly
    // transition
    if(true){
      state_transition(LORAWAN_INIT);
    }
    break;
  case LORAWAN_INIT:
    // defaults for timing out
    state_timeout_duration=1000;
    state_next=INIT;
    // transition
    // if initialization successful, move forward
    if(lorawan_init()){
      state_transition(LORAWAN_JOIN);
    }
    else{
      // TODO: decide what to do if LoraWAN fails
      // currently the state will auto-retry until timeout
    }
    break;
  case LORAWAN_JOIN:
    // defaults for timing out
    state_timeout_duration=60000;
    state_next=INIT;
    // transition
    if(lorawan_joined()){
      state_transition(GENERAL_INIT);
    }
    else{
      // TODO: create a more elaborate timeout mechanism
      // For now retry for 60s until timeout, which goes back to init
      sleep=1000;
    }
    break;
  case GENERAL_INIT:
    // defaults for timing out
    state_timeout_duration=10000;
    state_next=IDLE;
    // setup default settings
    status_init(); // currently does not report a fail, should not be possible anyhow
    event_status_last=millis();
    sensor_init(); // currently does not report a fail, TODO
    sensor_send_flag = true;
    event_sensor_last=millis();
    sensor_gps_init(); // self disables if fail present
    // transition
    if(true){
      state_transition(IDLE);
    }
    break;
  case IDLE:
    // defaults for timing out
    state_timeout_duration=25*60*60*1000; // 25h maximum
    state_next=INIT;
    // transition based on triggers
    if(settings_updated|settings_send_flag|status_send_flag|sensor_send_flag){
      if(settings_updated==true){
        state_transition(LORAWAN_INIT);
        settings_updated=false;
      }
      else if(settings_send_flag==true){
        state_transition(SETTINGS_SEND);
        settings_send_flag=false;
      }
      else if(sensor_send_flag==true){
        state_transition(GPS_START);
        sensor_send_flag=false;
      }
      else if(status_send_flag==true){
        state_transition(STATUS_SEND);
        status_send_flag=false;
      }
      else{
        // This should never happen
      }
    }
    else{
      // sleep until an event is generated
      sleep=0;
    }
    break;
  case SETTINGS_SEND:
    // defaults for timing out
    state_timeout_duration=2000;
    state_next=IDLE;
    // action
    // transition
    if(settings_send()){
      state_transition(LORAWAN_TRANSMIT);
    }
    else{
      sleep=1000;
    }
    break;
  case STATUS_SEND:
    // defaults for timing out
    state_timeout_duration=2000;
    state_next=IDLE;
    // transition
    if(status_send()){
      state_transition(LORAWAN_TRANSMIT);
    }
    else{
      sleep=1000;
    }
    break;
  case GPS_START:
    // defaults for timing out
    state_timeout_duration=0;
    state_next=GPS_READ;
    // action
    // transition
    if(sensor_gps_start()){
      state_transition(GPS_READ);
    }
    else{
      state_transition(SENSOR_READ);
    }
    break;
  case GPS_READ:
    // defaults for timing out
    state_timeout_duration=100*60*1000;
    state_next=IDLE;
    // action
    // transition
    if(sensor_gps_active==false){
      state_transition(SENSOR_READ);
    }
    else{
      // sleep for 1 second and check
      sleep=1000;
    }
    break;
  case SENSOR_READ:
    // defaults for timing out
    state_timeout_duration=0;
    state_next=SENSOR_SEND;
    // transition
    if(sensor_read()){
      state_transition(SENSOR_SEND);
    }
    else{
      // sleep for 1 second and check
      sleep=1000;
    }
    break;
  case SENSOR_SEND:
    // defaults for timing out
    state_timeout_duration=2000;
    state_next=IDLE;
    // transition
    if(sensor_send()){
      state_transition(LORAWAN_TRANSMIT);
    }
    else{
      // sleep for 1 second and check
     sleep=1000;
    }
    break;
  case LORAWAN_TRANSMIT:
    // defaults for timing out, transmission should not take more then 10s
    state_timeout_duration=10000;
    state_next=LORAWAN_INIT;
    // transition
    // if tx fails, reinit lorawan
    if(lorawan_send_successful){
      state_transition(IDLE);
    }
    else{
      sleep=1000;
    }
    break;
  case HIBERNATION:
    // defaults for timing out
    state_timeout_duration=0;
    state_next=IDLE;
    // action
    // TODO: implement this feature
    // transition
    if(true){
      state_transition(IDLE);
    }
    break;
  default:
    state=IDLE;
    break;
  }

  // check if the existing state has timed out and transition to next state
  if(state_check_timeout()){
    state_transition(state_next);
  }

  #ifdef debug
    serial_debug.print("fsm(");
    serial_debug.print(state);
    serial_debug.print(",");
    serial_debug.print(sleep);
    serial_debug.print(",");
    serial_debug.print(millis());
    serial_debug.println(")");
  #endif

  // reset the event loop start to show the loop has finished
  event_loop_start = 0;
  if(sleep>0){
    unsigned long sleep_temp=sleep;
    sleep=0;
    STM32L0.stop(sleep_temp);
  }
  else if(sleep==0){
    sleep=-1;
    STM32L0.stop();
  }
  else{
    sleep=-1;
  }
}
