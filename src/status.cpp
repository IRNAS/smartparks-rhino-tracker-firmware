#include "status.h"
#include <ponsel_sensor.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

uint8_t resetCause = 0xff;
Adafruit_BME280 bme; // comm over I2C

#define debug
#define serial_debug Serial

struct pinout pins =
{
    .boost_en = BOOST_EN,
    .uart_inh = UART_INH,
    .uart_sel_a = UART_SEL_A,
    .uart_sel_b =  UART_SEL_B,
    .driver_en = DRIVER_EN,
    .enable_driver = LOW,
    .disable_driver = HIGH,
};

ponsel_sensor ctzn(Serial1, 50, CTZN, pins);
ponsel_sensor optod(Serial1, 10, OPTOD, pins);

static bool ctzn_present = false;
static bool optod_present = false;
static bool bme_present = false;

boolean status_send_flag = false;
boolean status_dropoff_flag = false;
unsigned long event_status_last = 0;
unsigned long event_status_voltage_last = 0;
statusPacket_t status_packet;

extern charging_e charging_state;

TimerMillis timer_pulse_off;

LIS2DW12CLASS lis;

Adafruit_ADS1015 ads;     /* Use thi for the 12-bit version */

boolean pulse_state = LOW;
unsigned long pulse_last_time = 0;
uint8_t pulse_counter = 0;

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
  //Serial.print("PULSE DETECTED");
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
  if(status_dropoff_flag){
    status_dropoff_flag=false;
    status_dropoff();
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

#if defined DROP_CHG && defined DROP_EN

  pinMode(DROP_CHG, OUTPUT);
  digitalWrite(DROP_CHG,LOW);
  pinMode(DROP_EN, OUTPUT);
  digitalWrite(DROP_EN,LOW);

  ads.begin();

#endif //DROP_CHG DROP_EN

    if (bme.begin(0x76)) {
#ifdef serial_debug
        serial_debug.println("BME280 present!");
#endif 
        bme_present = true;
    }
    else {
#ifdef serial_debug
        serial_debug.println("BME280 not present!");
        serial_debug.println("I2C lines might not work!");
#endif 
    }

  // Needed for communication with ponsel sensors
#ifdef serial_debug
    serial_debug.println("Checking for ponsel sensors...");
#endif
  Serial1.begin(9600);
  if (ctzn.begin()) {
#ifdef serial_debug
    serial_debug.println("Ctzn present");
#endif
    ctzn_present = true;
  }
  else {
#ifdef serial_debug
    serial_debug.println("Ctzn not present");
#endif
  }
  if (optod.begin()) {
#ifdef serial_debug
    serial_debug.println("Optod present");
#endif
    optod_present = true;
  } 
  else {
#ifdef serial_debug
    serial_debug.println("Optod not present");
#endif
  }
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
  if (0!=status_packet.data.lat1) {
    status_packet.data.gps_resend++;
  }

    float bme_temp      = bme.readTemperature();
    float bme_pressure  = bme.readPressure() / 100;
    float bme_humid     = bme.readHumidity();

#ifdef debug
    serial_debug.print("bme_temp: ");
    serial_debug.println(bme_temp);
    serial_debug.print("bme_pressure: ");
    serial_debug.println(bme_pressure);
    serial_debug.print("bme_humid: ");
    serial_debug.println(bme_humid);
#endif

    if (bme_present) {
        status_packet.data.bme_temp =       (uint8_t) get_bits(bme.readTemperature(), -40, 85, 8);
        status_packet.data.bme_pressure =   (uint8_t) get_bits(bme.readPressure() / 100.0F, 300, 1100, 8);
        status_packet.data.bme_humid =      (uint8_t) get_bits(bme.readHumidity(), 0, 100, 8);
    }

    struct measurements values;
    if (ctzn_present)
    {
        if (ctzn.read_measurements()) {
#ifdef debug
            ctzn.print_measurements();
#endif
            values = ctzn.get_measurements();
            status_packet.data.ctzn_temp =            (uint8_t) get_bits(values.param1, 0, 40, 8);
            status_packet.data.conductivity_ctz =     (uint16_t)get_bits(values.param2, 0,100,16);
            status_packet.data.salinity =             (uint16_t)get_bits(values.param3, 5, 60,16);
            status_packet.data.conductivity_no_comp = (uint16_t)get_bits(values.param4, 0,100,16);
        }
    }


    if (optod_present)
    {
        if (optod.read_measurements()) {
#ifdef debug
            optod.print_measurements();
#endif
            values = optod.get_measurements();
            status_packet.data.optod_temp =     (uint8_t) get_bits(values.param1, 0, 40, 8);
            status_packet.data.oxygen_sat =     (uint16_t)get_bits(values.param2, 0,200,16);
            status_packet.data.oxygen_mgL =     (uint16_t)get_bits(values.param3, 5, 20,16);
            status_packet.data.oxygen_ppm =     (uint16_t)get_bits(values.param4, 0, 20,16);
        }
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

void status_fence_monitor_calibrate(uint16_t calibrate_value){
  //calculate and apply calibration
  float factor = calibrate_value;
  factor=factor/10000;
  calibration_packet.data.ads_calib =  factor;

#ifdef debug
  serial_debug.println(calibrate_value);
  serial_debug.println(calibration_packet.data.ads_calib);
#endif
  for(int i=0;i<sizeof(calibrationData_t);i++){
    EEPROM.write(EEPROM_DATA_START_SETTINGS+i,calibration_packet.bytes[i]);
  }
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
  float raw=0;
  float cumulative=0;
  float peak=0;
  float peak_average=0;
  float sample_counter_low=0;
  float sample_counter_high=0;
  float sample_test_length = 10;

  bool pulse_active = false;


  // start by assuming we are not in the middle of the pulse
  // the function below wait for 100ms
  int counter = 0; // when 100 samples in a row are 0, then there is for sure no pulse
  while(millis()<(start+1000)){
    raw = analogRead(VSWR_ADC)-20;
    raw=raw*calibration_packet.data.ads_calib;
    if(raw==0){
      counter++;
    }
    if(counter>10){
      break;
    }
  }
  //Serial.println("Pulse start");
  start = millis();
  // read values for 10s
  while(millis()<(start+10000)){
    raw = analogRead(VSWR_ADC);
    raw = raw*calibration_packet.data.ads_calib;
    // lower threshold to cut off noise
    if(raw<12){
      raw=0;
    }
    // track cumulative pulse energy and peak
    cumulative+=raw;
    peak=max(peak,raw);

    // track how many samples in a row are 0 or not
    if(raw>0){
      sample_counter_high++;
      sample_counter_low=0;
    }
    else{
      sample_counter_low++;
      sample_counter_high=0;
    }

    // if we are not in a pulse and sample_test_length(100) samples are of pulse value, go to pulse
    if(pulse_active==false){
      if(sample_counter_high>sample_test_length){
        // go to pulse active staet
        pulse_active=true;
        sample_counter_high=0;
      }
    }
    // else we are in a pulse and sample_test_length(100) samples are of 0 value, go to no pulse
    else{
      if(sample_counter_low>sample_test_length){
        // go to pulse not active
        pulse_active=false;
        sample_counter_low=0;
        //peak_average+=peak;
        //peak=0;
        pulse_counter++;
      }
    }
    // stop after 5 detected pulses
    if(pulse_counter>=5){
      break;
    }
  }

  // check if any pulses have been detected
  if(pulse_counter>0){
    //peak_average=peak_average/pulse_counter;

    cumulative=cumulative/pulse_counter/100;
    cumulative=min(cumulative,0x0fff);
  }
  else{
    //peak_average=0;
    cumulative=0;
  }

  peak_average=min(peak,0x0fff);
  //Serial.print("cumo: "); Serial.println(cumulative);
  //Serial.print("peak: "); Serial.println(peak_average);

  // data transmission is split into 
  // 8 bits for counter
  // 12 bits for peak
  // 12 bits for energy
  // This is packed into existing variables, which is not very clean, but backwards compatible
  status_packet.data.pulse_count=(uint8_t)pulse_counter;
  uint16_t pulse_energy = (uint16_t)cumulative;
  uint16_t pulse_voltage = (uint16_t)peak_average;

  status_packet.data.pulse_energy=pulse_energy>>4;
  status_packet.data.pulse_voltage=((pulse_energy<<12)&0xf000) | (pulse_voltage & 0x0fff);
  digitalWrite(ADS_EN,LOW);
  pinMode(ADS_EN, INPUT);
#endif //ADS_EN
}

void status_dropoff(){
// this is a prototype impelmentation, needs a rewrite

#if defined DROP_CHG && defined DROP_EN

// send a lora message to show the process start
uint8_t packet[2]={0xd0,0x00};
lorawan_send(99,&packet[0], sizeof(packet));
delay(3000);

  // set up the reqired state
  digitalWrite(DROP_CHG,LOW);
  digitalWrite(DROP_EN,LOW);
  // read battery and capacitor voltage

  int16_t capacitor, charge;

  capacitor = ads.readADC_SingleEnded(0);//*3;
  charge = ads.readADC_SingleEnded(1);//*3*1.846;

  #ifdef debug
  serial_debug.print("capacitor mv: "); serial_debug.println(capacitor);
  serial_debug.print("charge mv: "); serial_debug.println(charge);
  #endif

  //charge the capacitor
  pinMode(DROP_CHG,OUTPUT);
  digitalWrite(DROP_CHG,HIGH);

  unsigned long start = millis();
  // charge for 600s or less if voltage is reached sooner
  while(millis()<(start+(600000))){
    capacitor = ads.readADC_SingleEnded(0)*3;
    charge = ads.readADC_SingleEnded(1)*3*1.846;
    #ifdef debug
    serial_debug.print("capacitor mv: "); serial_debug.println(capacitor);
    serial_debug.print("charge mv: "); serial_debug.println(charge);
    #endif

    if(capacitor>2800){
      break;
    }
    STM32L0.wdtReset(); // reset watchdog
    STM32L0.stop(10000); // sleep for 10s
  }
  #ifdef debug
  serial_debug.print("charged to mv: "); serial_debug.println(capacitor);
  #endif

  //stop charging the capacitor
  digitalWrite(DROP_CHG,LOW);

  start = millis();
  // discharge for 5s or less if voltage is reached sooner

  //trigger the drop-off mechanism
  digitalWrite(DROP_EN,HIGH);
  STM32L0.stop(500); // sleep for 100ms

  while(millis()<(start+10000)){
    capacitor = ads.readADC_SingleEnded(0)*3;
    #ifdef debug
    serial_debug.print("capacitor mv: "); serial_debug.println(capacitor);
    #endif

    if(capacitor<900){
      break;
    }
    STM32L0.wdtReset(); // reset watchdog
    STM32L0.stop(100); // sleep for 100ms
    digitalWrite(DROP_EN,LOW);
    STM32L0.stop(250); // sleep for 100ms
    digitalWrite(DROP_EN,HIGH);
  }

  //stop the drop-off mechanism
  digitalWrite(DROP_EN,LOW);

  //send lora message to denote the end
  packet[1]=1;
  lorawan_send(99,&packet[0], sizeof(packet));
  delay(3000);

  #ifdef debug
  serial_debug.println("drop done");
  #endif
#endif
}
