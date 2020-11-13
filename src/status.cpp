#include "status.h"

uint8_t resetCause = 0xff;

// #define debug
// #define serial_debug  Serial

boolean status_send_flag = false;
unsigned long event_status_last = 0;
unsigned long event_status_voltage_last = 0;
statusPacket_t status_packet;

extern charging_e charging_state;

TimerMillis timer_pulse;

LIS2DW12CLASS lis;

boolean pulse_state = LOW;
uint8_t pulse_counter = 0;
unsigned long pulse_last_time = 0;

void pulse_output_off_callback(){
  #ifdef debug
    serial_debug.println("SD POWER OFF");
  #endif
  #ifdef PULSE_OUT
    digitalWrite(PULSE_OUT, LOW);
  #endif // PULSE_OUT
}

void pulse_output_on_callback(){
  #ifdef debug
    serial_debug.println("SD POWER ON");
  #endif
  // schedule a delayed pulse off to power off the SDCard
  timer_pulse.start(pulse_output_off_callback, settings_packet.data.pulse_on_timeout * 1000);
  #ifdef PULSE_OUT
    digitalWrite(PULSE_OUT, HIGH);
  #endif // PULSE_OUT
}

/*
pulse callback does the following
- count up the pulse counter on every rising edge
- when a number of pulses is reached, trigger sending a message
- on falling edge determine what to do
- if minimal interval between output pulses conditions is OK, then create an output pulse of specified length
- if a pulse occurs during this, count up and do nothing
*/
void pulse_callback(){
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
    pulse_counter++;

    boolean trigger_output = pulse_last_time == 0; // On first pulse always trigger the output

    // if number of pulses threshold has been reached, trigger output
    if(settings_packet.data.pulse_threshold > 0 && pulse_counter >= settings_packet.data.pulse_threshold){
      // do output
      trigger_output = true;
    }

    // if enough has passed since last pulse
    uint32_t pulse_interval = millis() - pulse_last_time;
    
    if(pulse_interval >= (settings_packet.data.pulse_min_interval * 1000)){
      // do output
      trigger_output = true;
    }
    
    // When we actually want to trigger but the timer is running, then skip the trigger because have already triggered recently.
    if (trigger_output && !timer_pulse.active()) {
      pulse_last_time = millis();
      pulse_counter = 0;
      // send LoRa message
      status_send_flag = HIGH;

      // turn on output to power the SDCard, do this delayed to give the Lora packet time to trigger and boot the RPi
      timer_pulse.start(pulse_output_on_callback, 45 * 1000);
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
serial_debug.print("p int: ");
serial_debug.print(settings_packet.data.pulse_min_interval);
serial_debug.print("p thr: ");
serial_debug.print(settings_packet.data.pulse_threshold);
serial_debug.print(" p to: ");
serial_debug.print(settings_packet.data.pulse_on_timeout);
serial_debug.print(" p act: ");
serial_debug.print(timer_pulse.active());
serial_debug.println(" )");
#endif

  // // TODO: For now this will be commented out
  // as it is sending status packages, Tue 14 Jul 2020 15:54:50 CEST
  //unsigned long elapsed = millis()-event_status_last;
  //if(elapsed>=(settings_packet.data.system_status_interval*60*1000)){
  //  event_status_last=millis();
  //  status_send_flag = true;
  //}
}

/**
 * @brief Initialize the status logic
 * 
 */
void status_init(void){ 
    event_status_last=millis();
    status_accelerometer_init();
    #ifdef debug
        serial_debug.print("status_init - status_timer_callback( ");
        serial_debug.print("interval: ");
        serial_debug.print(settings_packet.data.system_status_interval);
        serial_debug.println(" )");
    #endif

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
  // disable charging before measurement
  boolean charge_disabled=LOW;
#ifdef CHG_DISABLE
    pinMode(CHG_DISABLE, OUTPUT);
    digitalWrite(CHG_DISABLE, HIGH);
#endif // CHG_DISABLE
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

#ifdef CHG_DISABLE
  switch_charging_state();
#endif // CHG_DISABLE
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
  status_packet.data.system_functions_errors=status_packet.data.system_functions_errors|(charging_state<<5);

  accel_data axis;
  axis = status_accelerometer_read();

  status_packet.data.accelx=(uint8_t)get_bits(axis.x_axis,-2000,2000,8);
  status_packet.data.accely=(uint8_t)get_bits(axis.y_axis,-2000,2000,8);
  status_packet.data.accelz=(uint8_t)get_bits(axis.z_axis,-2000,2000,8);

  status_packet.data.pulse_count=pulse_counter;

  // increment prior to sending if valid data is there
  if(0!=status_packet.data.lat1){
    status_packet.data.gps_resend++;
  }

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

/**
 * @brief initialize sensors upon boot or new settings
 * 
 * @details Make sure each sensors is firstly properly reset and put into sleep and then if enabled in settings, initialize it
 * 
 */
void status_accelerometer_init(void){
    #ifdef debug
    serial_debug.print("sensor_accelerometer_init(");
    serial_debug.println(")");
  #endif

  if(lis.begin()==false){
      //set accelerometer error
      bitSet(status_packet.data.system_functions_errors,3);
      #ifdef debug
        serial_debug.print("accelerometer_init(");
        serial_debug.println("accel error)");
      #endif
      return;
  }
  else{
    bitClear(status_packet.data.system_functions_errors,3);
  }


  lis.wake_up_free_fall_setup(settings_packet.data.gps_triggered_threshold, settings_packet.data.gps_triggered_duration, 0xff);
}

accel_data status_accelerometer_read(){
  return lis.read_accel_values();
}
