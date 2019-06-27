#include "gps_tracker.h"

#define debug
#define serial_debug  Serial

/**
 * @brief change to next state and implement a timeout for each state
 * 
 */
void gps_state_transition(gps_state_e next){
  // mark the time state has been entered
  state_timeout_start = millis();
  // move to the following state
  state=next;
}

/**
 * @brief check if the state has timed out
 * 
 */
bool gps_state_check_timeout(void){
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

void gps_tracker_init(){
  gps_triggered = false;
}

void gps_tracker_main(){
  long sleep = -1; // reset the sleep after loop, set in every state if required
  event_loop_start = millis(); // start the timer of the loop
  #ifdef debug
    serial_debug.print("fsm(");
    serial_debug.print(state_prev);
    serial_debug.print(">");
    serial_debug.print(state);
    serial_debug.print(",");
    serial_debug.print(millis());
    serial_debug.println(")");
    serial_debug.flush();
  #endif

  // update prevous state
  state_prev=state;
  // FSM implementaiton
  switch (state)
  {
  case IDLE:
    // defaults for timing out
    state_timeout_duration=0;
    state_goto_timeout=INIT;
    if(gps_enabled==true){

    }
    else{
      state_transition(STOP);
    }
    
    break;
  case GENERAL_INIT:
    // defaults for timing out
    state_timeout_duration=0;
    state_goto_timeout=INIT;
    // LED diode
    state_transition(TRIGGERED_INIT);
    break;
  case PERIODIC_INIT:
    // defaults for timing out
    state_timeout_duration=0;
    state_goto_timeout=INIT;
    // LED diode
    state_transition(TRIGGERED_INIT);
    break;
  case TRIGGERED_INIT:
    // defaults for timing out
    state_timeout_duration=0;
    state_goto_timeout=INIT;
    // LED diode
    state_transition(LORAWAN_INIT);
    break;
  case WAKE_UP:
    // defaults for timing out
    state_timeout_duration=0;
    state_goto_timeout=INIT;
    // LED diode
    state_transition(LORAWAN_INIT);
    break;
  case START:
    // defaults for timing out
    state_timeout_duration=0;
    state_goto_timeout=INIT;
    // LED diode
    state_transition(LORAWAN_INIT);
    break;
  case RUN:
    // defaults for timing out
    state_timeout_duration=0;
    state_goto_timeout=INIT;
    // LED diode
    state_transition(LORAWAN_INIT);
    break;
  case SLEEP:
    // defaults for timing out
    state_timeout_duration=0;
    state_goto_timeout=INIT;
    // LED diode
    state_transition(LORAWAN_INIT);
    break;
  case STOP:
    // defaults for timing out
    state_timeout_duration=0;
    state_goto_timeout=INIT;
    // LED diode
    state_transition(LORAWAN_INIT);
    break;
  case DISABLED:
    // defaults for timing out
    state_timeout_duration=0;
    state_goto_timeout=IDLE;
    
    if(gps_enabled==true){
      state_transition(IDLE);
    }
    else{
      sleep=0;
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