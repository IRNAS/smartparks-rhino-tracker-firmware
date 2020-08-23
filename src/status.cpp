#include "status.h"

uint8_t resetCause = 0xff;

#define debug
#define serial_debug  Serial

boolean status_send_flag = false;
unsigned long event_status_last = 0;
unsigned long event_status_voltage_last = 0;
statusPacket_t status_packet;

extern charging_e charging_state;

TimerMillis timer_pulse_off;

LIS2DW12CLASS lis;

boolean pulse_state = LOW;
unsigned long pulse_last_time = 0;
uint8_t pulse_counter = 0;

// Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
Adafruit_ADS1015 ads;     /* Use thi for the 12-bit version */
boolean ads_present = false;


void pulse_output_on_callback(){
#ifdef PULSE_IN

#ifdef PULSE_OUT
  digitalWrite(PULSE_OUT, HIGH);
  //digitalWrite(LED_RED, HIGH);
#endif // PULSE_OUT
}

void pulse_output_off_callback(){
#ifdef PULSE_OUT
  // make sure if the input pin is high, that the output is not turned off
  if(digitalRead(PULSE_IN)){
    return;
  }
  digitalWrite(PULSE_OUT, LOW);
  //digitalWrite(LED_RED, LOW);
#endif // PULSE_OUT
}

/*
pulse callback does the following
- count up the pulse counter on every rising edge
- when a number of pulses is reached, trigger sending a message
- on falling edge determine what to do
- if minimal interval between output pulses conditions is ok, then create an output pulse of specified length
- if a pulse occurs during this, count up and do nothing
*/
void pulse_callback(){
  Serial.print("PULSE DETECTED");
  pulse_state = digitalRead(PULSE_IN);

  if(pulse_state==LOW){
    // if not delayed by timer, handle immediately
    if(timer_pulse_off.active()){
      //pass
    }
    else{
      pulse_output_off_callback();
    }
  }
  else{
    // turn on output
    pulse_output_on_callback();
    pulse_counter++;

    boolean trigger_output=false;

    // if number of pulses threshold has been reached, trigger output
    if(settings_packet.data.pulse_threshold<pulse_counter){
      // do output
      trigger_output=true;
    }

    // if enough has passed since last pulse
    if((millis()-pulse_last_time)>(settings_packet.data.pulse_min_interval*1000)){
      // do output
      trigger_output=true;
      pulse_last_time=millis();
    }

    if(trigger_output){
      // send lora message
      status_send_flag=HIGH;
      // schedule a delayed pulse off
      //timer_pulse_off.stop();
      timer_pulse_off.start(pulse_output_off_callback, settings_packet.data.pulse_on_timeout*1000);
    }
  }
#endif //PULSE_IN
}

/**
 * @brief Schedule status events
 * 
 */
void status_scheduler(void){

#ifdef debug
serial_debug.print("status_scheduler -( ");
serial_debug.print("p thr: ");
serial_debug.print(settings_packet.data.pulse_threshold);
serial_debug.print(" p to: ");
serial_debug.print(settings_packet.data.pulse_on_timeout);
serial_debug.print(" p min: ");
serial_debug.print(timer_pulse_off.active());
serial_debug.println(" )");
#endif
  unsigned long elapsed = millis()-event_status_last;
  if(elapsed>=(settings_packet.data.system_status_interval*60*1000)){
    event_status_last=millis();
    status_send_flag = true;
  }
}

/**
 * @brief Initialize the staus logic
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

#ifdef ADS_EN
  pinMode(ADS_EN, OUTPUT);
  digitalWrite(ADS_EN,LOW);
#endif // ADS_EN

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
  status_packet.data.battery=(uint8_t)stm32l0_battery; // 0-5000 input, assuming 2500mV is minimu that is subtracted and then divided by 10
  status_packet.data.temperature=(uint8_t)get_bits(stm32l0_temp,-20,80,8);
  status_packet.data.input_voltage=input_voltage;
}

/**
 * @brief Send stauts values
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
  pulse_counter=0;

  status_fence_monitor_read();

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
 * @details Make sure each sensors is firstlu properly reset and put into sleep and then if enabled in settings, initialize it
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

void status_fence_monitor_read(){
/*
1) read analog samples until pulse is DETECTED
2) check pulse data to see if pulse is valid
3) measure a few pulses and return the period between them
*/
#ifdef ADS_EN
  // LoRaWAN.sendPacket( function changes PB14 pin to input for some reason
  pinMode(ADS_EN, OUTPUT);
  digitalWrite(ADS_EN,HIGH);
  delay(10);
  unsigned long start = millis();
  int raw=0;
  float cumulative=0;
  float peak=0;
  float peak_average=0;
  float sample_counter=0;

  bool pulse_active = false;

  // start by assuming we are not in the middle of the pulse
  // the function below wait for 100ms
  int counter = 0; // when 100 samples in a row are 0, then there is for sure no pulse
  while(millis()<(start+1000)){
    raw = analogRead(VSWR_ADC)-100;
    if(raw==0){
      counter++;
    }
    if(counter>100){
      break;
    }
  }
  Serial.println("Pulse start");
  start = millis();
  // read values for 10s
  while(millis()<(start+10000)){
    raw = analogRead(VSWR_ADC)-100;

    if(pulse_active==true){
      // we are in the pulse
      if(raw>0){
        cumulative+=raw;
        peak=max(peak,raw);
        sample_counter=0;
      }
      else{
        // wait for 100 samples of no pulse
        sample_counter++;
        if(sample_counter>100){
          pulse_active=false;
          pulse_counter++;
          peak_average+=peak;
          Serial.print("Pulse ");
          Serial.print(pulse_counter);
          Serial.print(" ");
          Serial.println(peak);
          peak=0;
        }
      }

    }
    else{
      // we are waiting for the pulse
      if(raw>0){
        // go to pulse active mode
        pulse_active=true;
        peak=max(peak,raw);
        sample_counter=0;
      }
    }
    if(pulse_counter>=5){
      break;
    }
  }

  if(pulse_counter>0){
    peak_average=peak_average/pulse_counter;
    cumulative=cumulative/pulse_counter;
  }
  else{
    peak_average=0;
    cumulative=0;
  }
  Serial.print("cumo: "); Serial.println(cumulative);
  Serial.print("peak: "); Serial.println(peak_average);

  status_packet.data.pulse_count=(uint8_t)pulse_counter; // not yet implemented
  float energy = 0;
  //limit peak to 255
  peak_average=min(peak_average/10-20,0xff);
  status_packet.data.pulse_energy=(uint8_t)peak_average;
  status_packet.data.pulse_voltage=(uint16_t)(cumulative/100);
  digitalWrite(ADS_EN,LOW);
  pinMode(ADS_EN, INPUT);
#endif //ADS_EN
}

