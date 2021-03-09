#include "status.h"

uint8_t resetCause = 0xff;

#define debug
#define serial_debug  Serial

boolean status_send_flag = false;
unsigned long event_status_last = 0;
unsigned long event_status_voltage_last = 0;
statusPacket_t status_packet;

TimerMillis timer_pulse;
TimerMillis timer_debounce;

boolean pulse_state = LOW;
unsigned long debounce_start = 0;

void pulse_output_off_callback() {
  #ifdef debug
    serial_debug.println("SD POWER OFF");
  #endif
  #ifdef PULSE_OUT
    digitalWrite(PULSE_OUT, LOW);
  #endif // PULSE_OUT
}

void pulse_output_on_callback() {
  #ifdef debug
    serial_debug.println("SD POWER ON");
  #endif
  // schedule a delayed pulse off to power off the SDCard
  timer_pulse.start(pulse_output_off_callback, settings_packet.data.sd_power_time * 1000);
  #ifdef PULSE_OUT
    digitalWrite(PULSE_OUT, HIGH);
  #endif // PULSE_OUT
}

void trigger_output() {
  #ifdef debug
    serial_debug.println("Trigger output");
  #endif
  
  status_send_flag = HIGH; // send LoRa message

  // turn on output to power the SDCard, do this delayed to give the Lora packet time to trigger and boot the RPi
  timer_pulse.start(pulse_output_on_callback, settings_packet.data.sd_power_delay * 1000);
}

/*
pulse callback
*/
void pulse_callback() {
  pulse_state = digitalRead(PULSE_IN);
  uint32_t start_of_pulse = millis();

  while(digitalRead(PULSE_IN) == 1);
  uint32_t end_of_pulse = millis();
  
  status_packet.data.duration_of_pulse = (end_of_pulse - start_of_pulse);
  
  #ifdef debug
    Serial.print("PULSE DETECTED: ");
    Serial.println(status_packet.data.duration_of_pulse);
  #endif

  // Only set send flag, if duration of pulse is longer 1900 ms, this will
  // filter out all pulses that were not result of detected PIR activity
  if(status_packet.data.duration_of_pulse > 1900){
    
    // When power on the sd card is on (or waiting to turn on) skip this trigger
    if (!timer_pulse.active()) {
      if(timer_debounce.active()) {
        // Debounce timer is already running.
        // Let's see if our max timeout has elapsed, this is the maximum time we will allow delaying 
        // the output trigger to prevent the output to be delayed indefinitely.

        if(millis() - debounce_start > settings_packet.data.max_debounce_time * 1000) {
          #ifdef debug
            serial_debug.println("Max wait reached");
          #endif
          // Maximum wait time has been reached, trigger the output immediately
          timer_debounce.stop();
          trigger_output();
        } else {
          #ifdef debug
            serial_debug.println("Restart debounce");
          #endif
          // Restart the timer to extend the wait delay
          timer_debounce.restart(settings_packet.data.debounce_time * 1000);
        }
      } else {
        #ifdef debug
          serial_debug.println("Start debounce");
        #endif
        // Start a timer to see and wait if more triggers will occur in debounce_time
        debounce_start = millis();
        if (settings_packet.data.debounce_time <= 0) {
          trigger_output();
        } else {
          timer_debounce.start(trigger_output, settings_packet.data.debounce_time * 1000);
        }
      }
    }
  }
}

/**
 * @brief Schedule status events
 * 
 */
void status_scheduler(void){
#ifdef debug
serial_debug.print("status_scheduler -( ");
serial_debug.print("p db: ");
serial_debug.print(settings_packet.data.debounce_time);
serial_debug.print("p mdb: ");
serial_debug.print(settings_packet.data.max_debounce_time);
serial_debug.print("p to: ");
serial_debug.print(settings_packet.data.sd_power_time);
serial_debug.print("p act: ");
serial_debug.print(timer_pulse.active());
serial_debug.println(" )");
#endif
}

/**
 * @brief Initialize the status logic
 * 
 */
void status_init(void){ 
    event_status_last=millis();

#ifdef PULSE_IN
  pinMode(PULSE_IN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(PULSE_IN), pulse_callback, RISING);
#endif // PULSE_IN
#ifdef PULSE_OUT
  pinMode(PULSE_OUT, OUTPUT);
  digitalWrite(PULSE_OUT,LOW);
#endif // PULSE_OUT
}

/**
 * @brief measure battery and input voltage
 * 
 */
void status_measure_voltage(){
  float stm32l0_temp = STM32L0.getTemperature();
  float stm32l0_vdd = STM32L0.getVDDA()*1000;
  float stm32l0_battery = 0;
#ifdef BAT_MON_EN
  // measure battery voltage
  pinMode(BAT_MON_EN, OUTPUT);
  digitalWrite(BAT_MON_EN, HIGH);
  delay(1);
  float value = 0;
  for(int i=0; i<16; i++){
    value+=analogRead(BAT_MON);
    delay(1);
  }
  stm32l0_battery = BAT_MON_CALIB * (value/16) * (2500/stm32l0_vdd);// result in mV
  stm32l0_battery=(stm32l0_battery-2500)/10;
  digitalWrite(BAT_MON_EN, LOW);
#endif //BAT_MON_EN

#ifdef debug
  serial_debug.print("status_measure_voltage( ");
  serial_debug.print(", battery: ");
  serial_debug.print(stm32l0_vdd);
  serial_debug.print(" ");
  serial_debug.print(stm32l0_battery);
  serial_debug.println(" )");
#endif

  // measure input voltage
  float input_voltage = 0;
  float input_value = 0;
#ifdef INPUT_AN
  if(INPUT_AN!=0){
    delay(1);
    input_value = 0;
    for(int i=0; i<16; i++){
      input_value+=analogRead(INPUT_AN);
      delay(1);
    }
    input_voltage = input_value/16;
  }
  uint8_t input_voltage_lookup_index = (uint8_t)(input_voltage/100);
  float input_calib_value=0;
  if(input_voltage_lookup_index<sizeof(input_calib)){
      input_calib_value=input_calib[input_voltage_lookup_index]*10;
  }
input_voltage=input_calib_value * input_voltage * (2500/stm32l0_vdd); // mV

#endif
  status_packet.data.battery=(uint8_t)stm32l0_battery; // 0-5000 input, assuming 2500mV is minimum that is subtracted and then divided by 10
  status_packet.data.temperature=(uint8_t)get_bits(stm32l0_temp,-20,80,8);
  status_packet.data.input_voltage=input_voltage;
}

/**
 * @brief Send status values
 * 
 * @return boolean 
 */
boolean status_send(void){
  //assemble information
  status_measure_voltage();
  status_packet.data.system_functions_errors=status_packet.data.system_functions_errors&0b00011111;

  status_packet.data.device_id=STM32L0.getSerial(); 

  #ifdef debug
    serial_debug.print("status_send( ");
    serial_debug.print("resetCause: ");
    serial_debug.print(STM32L0.resetCause(),HEX);
    serial_debug.print(", battery: ");
    serial_debug.print(status_packet.data.battery*10+2500);
    serial_debug.print(" input_voltage ");
    serial_debug.print(status_packet.data.input_voltage);
    serial_debug.print(", temperature: ");
    serial_debug.print(status_packet.data.temperature);
    serial_debug.println(" )");
  #endif

  return lorawan_send(status_packet_port, &status_packet.bytes[0], sizeof(statusData_t));
}