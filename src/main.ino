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

unsigned long event_loop_start = 0;

bool reed_switch = false;

enum state_e{
  INIT,
  LORAWAN_INIT,
  LORAWAN_JOIN,
  GENERAL_INIT,
  IDLE,
  SETTINGS_SEND,
  STATUS_SEND,
  GPS_START,
  GPS_RUN,
  GPS_READ,
  SENSOR_READ,
  SENSOR_SEND,
  LORAWAN_TRANSMIT,
  HIBERNATION
} ;

state_e state = INIT;
state_e state_goto_timeout;
state_e state_prev = INIT;
unsigned long state_timeout_start;
unsigned long state_timeout_duration;

// function prototypes because Arduino failes if using enum otherwise
void callbackPeriodic(void);
void callbackReed(void);
void state_transition(state_e next);
bool state_check_timeout(void);

void checkReed(void){
    // Reed switch
  /*pinMode(PIN_REED,INPUT_PULLUP);
  // debounce
  int counter_high = 0;
  int counter_low = 0;
  for(int i=0;i<10;i++){
    if(digitalRead(PIN_REED)==false){
      counter_low++;
    }
    else{
      counter_high++;
    }
    STM32L0.stop(1);
  }
  if(counter_low>=9){
    if(reed_switch!=true){
      STM32L0.wakeup();
    }
    reed_switch = true;
  }
  else if(counter_high>=9){
    if(reed_switch!=false){
      STM32L0.wakeup();
    }
    reed_switch = false;
  }
  // Reed switch
  pinMode(PIN_REED,INPUT_PULLDOWN);*/
}
/*
 *  Function:     void callbackPeriodic(void)
 *  Description:  function called with timer to kick the watchdog
 */

void callbackPeriodic(void){
  STM32L0.wdtReset();
  
  #ifdef debug
    serial_debug.print("wdt(): ");
    serial_debug.println(millis());
  #endif

  checkReed();

  // determine which events need to be scheduled, except in hibernation
  if(state!=HIBERNATION){
    status_scheduler();
    sensor_scheduler();
  }

  // if the main loop is running and not sleeping
  if(event_loop_start!=0){
    unsigned long elapsed = millis()-event_loop_start;
    // if loop has been running for more then 60s, then reboot system
    if(elapsed>=60*1000){
      STM32L0.reset();
    }
  }

  // if any of the flags are high, wake up
  if(settings_updated|status_send_flag|sensor_send_flag){
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
  periodic.start(callbackPeriodic, 00, 5000);

  // Serial port debug setup
  #ifdef serial_debug
    serial_debug.begin(115200);
  #endif
  #ifdef debug
    serial_debug.print("setup(serial debug begin): ");
    serial_debug.print("resetCause: ");
    serial_debug.println(STM32L0.resetCause(),HEX);
  #endif

  // start the FSM with LoraWAN init
  state = INIT;
}


void loop() {
  long sleep = -1;
  event_loop_start = millis();
  #ifdef debug
    serial_debug.print("fsm(");
    serial_debug.print(state_prev);
    serial_debug.print(">");
    serial_debug.print(state);
    serial_debug.print(",");
    serial_debug.print(sleep);
    serial_debug.print(",");
    serial_debug.print(millis());
    serial_debug.println(")");
    serial_debug.flush();
  #endif

  // update prevous state
  state_prev=state;
  // FSM implementaiton for clarity of process loop
  switch (state)
  {
  case INIT:
    // defaults for timing out
    state_timeout_duration=0;
    state_goto_timeout=INIT;
    // LED diode
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
    state_goto_timeout=INIT;
    // transition
    // if initialization successful, move forward
    if(lorawan_init()){
      state_transition(LORAWAN_JOIN);
    }
    else{
      // TODO: decide what to do if LoraWAN fails
      // Currently very harsh, doing full system reset
      STM32L0.reset();
    }
    break;
  case LORAWAN_JOIN:
    // defaults for timing out
    state_timeout_duration=60000;
    state_goto_timeout=INIT;
    // transition
    if(lorawan_joined()){
      state_transition(GENERAL_INIT);
      // LED diode
      digitalWrite(LED_RED,LOW);
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
    state_goto_timeout=IDLE;
    // setup default settings
    status_init(); // currently does not report a fail, should not be possible anyhow
    sensor_init(); // currently does not report a fail, TODO
    sensor_send_flag = true;
    status_send_flag = true;
    sensor_send_flag = true;
    //sensor_gps_init(); // self disables if fail present
    // transition
    if(true){
      state_transition(SETTINGS_SEND);
    }
    break;
  case IDLE:
    // defaults for timing out
    state_timeout_duration=25*60*60*1000; // 25h maximum
    state_goto_timeout=INIT;
    // LED diode
    digitalWrite(LED_RED,LOW);
    
    if(reed_switch){
      state_transition(HIBERNATION);
    }
    // transition based on triggers
    else if(settings_updated|status_send_flag|sensor_send_flag){
      if(settings_updated==true){
        state_transition(GENERAL_INIT);
        settings_updated=false;
      }
      else if(status_send_flag==true){
        state_transition(STATUS_SEND);
        status_send_flag=false;
      }
      else if(sensor_send_flag==true){
        state_transition(GPS_START);
        sensor_send_flag=false;
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
    state_goto_timeout=IDLE;
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
    state_goto_timeout=IDLE;
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
    state_timeout_duration=3000;
    state_goto_timeout=GENERAL_INIT;
    // action

    // transition
    // if GPS error, skip reading
    if(status_packet.data.system_functions_errors&0x03){
      state_transition(SENSOR_READ);
    }
    //if gps started continue
    else if(sensor_gps_start()){
      state_transition(GPS_RUN);
    }
    else{
      state_transition(SENSOR_READ);
    }
    break;
  case GPS_RUN:
    // defaults for timing out
    state_timeout_duration=3000;
    state_goto_timeout=GENERAL_INIT;
    // action
    // this state is here if gps does not send data and should reinit
    // transition
    if(sensor_gps_active==true){
      state_transition(GPS_READ);
    }
    else{
      // sleep for 1 second and check
      sleep=1000;
    }
    break;
  case GPS_READ:
    // defaults for timing out
    state_timeout_duration=20*60*1000;
    state_goto_timeout=IDLE;
    // action
    // transition
    if(sensor_gps_done==true){
      state_transition(SENSOR_READ);
    }
    else{
      // sleep for 1 second and check
      sleep=4000;
    }
    break;
  case SENSOR_READ:
    // defaults for timing out
    state_timeout_duration=0;
    state_goto_timeout=SENSOR_SEND;
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
    state_goto_timeout=IDLE;
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
    state_timeout_duration=15000;
    state_goto_timeout=LORAWAN_INIT;
    // transition
    // if tx fails, reinit lorawan
    if(lorawan_send_successful){
      state_transition(IDLE);
    }
    else{
      // tx successful flag is expected in about 3s after sending, note lora rx windows most complete
      sleep=5000;
    }
    break;
  case HIBERNATION:
    // defaults for timing out
    state_timeout_duration=24*60*60*1000; // 24h maximum
    state_goto_timeout=IDLE;
    // action
    if(reed_switch==false){
      state_transition(IDLE);
      // Send status immediately
      status_send_flag=HIGH;
    }
    else{
      sleep=0; // until an event
    }
    break;
  default:
    state=IDLE;
    break;
  }

  // check if the existing state has timed out and transition to next state
  if(state_check_timeout()){
    #ifdef debug
      serial_debug.print("timeout(");
      serial_debug.print(state);
      serial_debug.println(")");
    #endif
    state_transition(state_goto_timeout);
  }

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
