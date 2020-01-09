#include "status.h"

uint8_t resetCause = 0xff;

#define debug
#define serial_debug  Serial

boolean status_send_flag = false;
unsigned long event_status_last = 0;
unsigned long event_status_voltage_last = 0;
statusPacket_t status_packet;

/**
 * @brief Schedule status events
 * 
 */
void status_scheduler(void){
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
    #ifdef debug
        serial_debug.print("status_init - status_timer_callback( ");
        serial_debug.print("interval: ");
        serial_debug.print(settings_packet.data.system_status_interval);
        serial_debug.println(" )");
    #endif
}

/**
 * @brief measure battery and input voltage
 * 
 */
void status_measure_voltage(){
  float stm32l0_temp = STM32L0.getTemperature();
  float stm32l0_vdd = STM32L0.getVDDA();
  // disable charging before measurement
  boolean charge_disable=LOW;
#ifdef CHG_DISABLE
    charge_disable=digitalRead(CHG_DISABLE);
    //remember the state of charging
    pinMode(CHG_DISABLE, OUTPUT);
    digitalWrite(CHG_DISABLE, HIGH);
#endif // CHG_DISABLE

  // measure battery voltage
  pinMode(BAT_MON_EN, OUTPUT);
  digitalWrite(BAT_MON_EN, HIGH);
  delay(1);
  float value = 0;
  for(int i=0; i<16; i++){
    value+=analogRead(BAT_MON);
    delay(1);
  }
  float stm32l0_battery = value/16*BAT_MON_CALIB; // result in mV
  digitalWrite(BAT_MON_EN, LOW);

  // measure input voltage
  float input_voltage = 0;
#ifdef INPUT_AN
  if(INPUT_AN!=0){
    delay(1);
    value = 0;
    for(int i=0; i<16; i++){
      value+=analogRead(INPUT_AN);
      delay(1);
    }
    input_voltage = value/16;
  }
  uint8_t input_voltage_lookup_index = (uint8_t)input_voltage/100;
  float input_calib_value=1;
  if(input_voltage_lookup_index<sizeof(input_calib)){
      input_calib_value=input_calib[input_voltage_lookup_index]*10;
  }
  input_voltage=(input_calib_value * stm32l0_vdd * input_voltage) / 4095.0; // mV

#ifdef CHG_DISABLE
    pinMode(CHG_DISABLE, OUTPUT);
    digitalWrite(CHG_DISABLE, charge_disable);
#endif // CHG_DISABLE
#endif
  status_packet.data.battery=(uint8_t)(stm32l0_battery-2500)/10; // 0-5000 input, assuming 2500mV is minimu that is subtracted and then divided by 10
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
  
  status_packet.data.resetCause=STM32L0.resetCause();
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