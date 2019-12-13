#include "status.h"

uint8_t resetCause = 0xff;

//#define debug
//#define serial_debug  Serial

boolean status_send_flag = false;
unsigned long event_status_last = 0;
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
 * @brief Send stauts values
 * 
 * @return boolean 
 */
boolean status_send(void){
  //assemble information
  float voltage=100;//impelment reading voltage
  float stm32l0_vdd = STM32L0.getVDDA();
  float stm32l0_temp = STM32L0.getTemperature();

  pinMode(BAT_MON_EN, OUTPUT);
  digitalWrite(BAT_MON_EN, HIGH);
  delay(10);
  float value = 0;
  for(int i=0; i<16; i++){
    value+=analogRead(BAT_MON);
    delay(1);
  }
  float stm32l0_battery = value/16; // TODO: calibrate
  digitalWrite(BAT_MON_EN, LOW);
  //pinMode(BAT_MON_EN, INPUT_PULLDOWN);

  float stm32l0_battery_low = 0;

  #ifdef INPUT_AN
  if(INPUT_AN!=0){
    delay(10);
    value = 0;
    for(int i=0; i<16; i++){
      value+=analogRead(INPUT_AN);
      delay(1);
    }
    stm32l0_battery_low = value/16; // TODO: calibrate
  }
  #endif
  
  status_packet.data.resetCause=STM32L0.resetCause();
  status_packet.data.battery=(uint8_t)get_bits(stm32l0_battery,2048,4096,8);
  status_packet.data.battery_low=(uint8_t)get_bits(stm32l0_battery_low,0,4096,8);
  status_packet.data.temperature=(uint8_t)get_bits(stm32l0_temp,-20,80,8);
  status_packet.data.vbus=(uint8_t)get_bits(stm32l0_vdd,0,3.6,8);
  // increment prior to sending if valid data is there
  if(0!=status_packet.data.lat1){
    status_packet.data.gps_resend++;
  }

  #ifdef debug
    serial_debug.print("status_send( ");
    serial_debug.print("resetCause: ");
    serial_debug.print(STM32L0.resetCause(),HEX);
    serial_debug.print(", battery: ");
    serial_debug.print(status_packet.data.battery);
    serial_debug.print(" raw ");
    serial_debug.print(status_packet.data.battery_low);
    serial_debug.print(", temperature: ");
    serial_debug.print(stm32l0_temp);
    serial_debug.print(" ");
    serial_debug.print(status_packet.data.temperature);
    serial_debug.print(", stm32l0_vdd: ");
    serial_debug.print(stm32l0_vdd);
    serial_debug.print(" ");
    serial_debug.print(status_packet.data.vbus);
    serial_debug.println(" )");
  #endif

  return lorawan_send(status_packet_port, &status_packet.bytes[0], sizeof(statusData_t));
}