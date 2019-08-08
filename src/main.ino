#include <STM32L0.h> 
#include "TimerMillis.h"
#include "board.h"
#include "command.h"
#include "lorawan.h"
#include "gps_tracker.h"
#include "settings.h"
#include "status.h"
#include "project_utils.h"

#define debug
#define serial_debug  Serial

// Initialize timer for periodic callback
// TimerMillis periodic;
GNSSLocation gps_location;

/**
 * @brief called upon pin change
 * 
 */
void accelerometer_callback(void){
  /*#ifdef debug
    serial_debug.print("gps_accelerometer_callback(");
    serial_debug.println(")");
  #endif */
  gps_accelerometer_interrupt();
}

// Variable to track the reed switch status
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
  GPS_READ,
  GPS_SEND,
  LORAWAN_TRANSMIT,
  HIBERNATION
};

state_e state = INIT;
state_e state_goto_timeout;
state_e state_prev = INIT;
unsigned long state_timeout_start;
unsigned long state_timeout_duration;
// Variable to monitor when the loop has been started
unsigned long event_loop_start = 0;
long sleep = -1; // reset the sleep after loop, set in every state if required

// function prototypes because Arduino failes if using enum otherwise
boolean callbackPeriodic(void);
void state_transition(state_e next);
bool state_check_timeout(void);

/**
 * @brief Check reed switch status
 * 
 */
void checkReed(void){
  pinMode(PIN_REED,INPUT_PULLUP);
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
    delay(1);
  }
  if(counter_low>=9){
    reed_switch = true;
  }
  else if(counter_high>=9){
    reed_switch = false;
  }
  // Reed switch
  pinMode(PIN_REED,INPUT_PULLDOWN);
}

/**
 * @brief Callback ocurring periodically for triggering events and wdt
 * 
 */
boolean callbackPeriodic(void){
  //periodic.start(callbackPeriodic, 5000);
  STM32L0.wdtReset();
  
  /*#ifdef debug
    serial_debug.print("wdt(): ");
    serial_debug.println(millis());
  #endif*/

  // determine which events need to be scheduled, except in hibernation
  if(state!=HIBERNATION){
    status_scheduler();
    gps_scheduler();
  }
  else{
    gps_send_flag = false;
    status_send_flag = false;
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
  if(settings_updated|status_send_flag|gps_send_flag){
    //STM32L0.wakeup();
    return true;
    /*#ifdef debug
      serial_debug.print("wakeup(");
      serial_debug.print(millis());
      serial_debug.println(")");
    #endif*/
  }
  return false;
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

/**
 * @brief Setup function called on boot
 * 
 */
void setup() {
  //STM32L0.stop(60000); //limits the reboot continuous cycle from happening for any reason, likely low battery
  // Watchdog
  STM32L0.wdtEnable(18000);
  analogReadResolution(12);

  pinMode(A_INT2, INPUT);
  attachInterrupt(digitalPinToInterrupt(A_INT2),accelerometer_callback,RISING);

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

/**
 * @brief Main system loop running the FSM
 * 
 */
void loop() {
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
  sleep = -1; // reset the sleep after loop, set in every state if required
  event_loop_start = millis(); // start the timer of the loop
  callbackPeriodic();

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
    checkReed();
    if(reed_switch){
      state_transition(HIBERNATION);
    }
    else{
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
    // Accelerometer
    accelerometer_init();
    status_send_flag = true;
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
    
    checkReed();
    if(reed_switch){
      // Resets the system, expect goign straight to Hiberantion
      STM32L0.reset();
    }
    // transition based on triggers
    else if(settings_updated|status_send_flag|gps_send_flag){
      if(settings_updated==true){
        state_transition(GENERAL_INIT);
        settings_updated=false;
      }
      else if(status_send_flag==true){
        state_transition(STATUS_SEND);
        status_send_flag=false;
      }
      else if(gps_send_flag==true){
        state_transition(GPS_START);
        gps_send_flag=false;
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
    state_timeout_duration=0;
    state_goto_timeout=GPS_SEND;
    //if gps started continue
    if(gps_start()){
      state_transition(GPS_READ);
    }
    else{
      state_transition(STATUS_SEND);
    }
    break;
  case GPS_READ:
    // defaults for timing out
    state_timeout_duration=610*1000;
    state_goto_timeout=GPS_SEND;
    //if gps started continue
    if(gps_done==true){
      //if GPS error, send status
      if(status_packet.data.system_functions_errors&0x03){
        // send status instead of GPS
        status_send_flag=true;
        state_transition(IDLE);
      }
      else{
        state_transition(GPS_SEND);
      }
    }
    else{
      sleep=4000;
    }
    break;
  case GPS_SEND:
    // defaults for timing out
    state_timeout_duration=2000;
    state_goto_timeout=IDLE;
    // transition
    if(gps_send()){
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
    state_goto_timeout=INIT;
    // action
    checkReed();
    if(reed_switch==false){
      state_transition(INIT);
      // Trigger all events
      gps_send_flag = true;
      status_send_flag = true;
      gps_send_flag = true;
    }
    else{
      sleep=60000; // until an event
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
    system_sleep(sleep);
    sleep=0;
  }
  else if(sleep==0){
    sleep=-1;
    system_sleep(25*3600*1000); // max 25h
  }
  else{
    sleep=-1;
  }
}

void system_sleep(unsigned long sleep){
  unsigned long remaining_sleep = sleep;
  callbackPeriodic();
  while(remaining_sleep>0){
    if(remaining_sleep>5000){
      remaining_sleep=remaining_sleep-5000;
      STM32L0.stop(5000);
    }
    else{
      STM32L0.stop(remaining_sleep);
      remaining_sleep=0;
    }
    //wake-up
    if(callbackPeriodic()){
      return;
    }
  }
}
