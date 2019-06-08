#include "status.h"

uint8_t resetCause = 0xff;

//#define debug
//#define serial_debug  Serial

boolean status_send_flag = false;
statusPacket_t status_packet;

void status_timer_callback(void)
{
  #ifdef debug
    serial_debug.println("status_timer_callback()");
  #endif
  status_send_flag = true;
}

void status_init(void){ 
    //status_timer.stop();
    //status_timer.start(status_timer_callback, 0, settings_packet.data.system_status_interval*60*1000);
    #ifdef debug
        serial_debug.print("status_init - status_timer_callback( ");
        serial_debug.print("interval: ");
        serial_debug.print(settings_packet.data.system_status_interval);
        serial_debug.println(" )");
    #endif
}

void status_send(void){
    //assemble information
    float voltage=100;//impelment reading voltage
    float stm32l0_vdd = STM32L0.getVBUS();
    float stm32l0_temp = STM32L0.getTemperature();

    status_packet.data.resetCause=STM32L0.resetCause();
    status_packet.data.battery=(uint8_t)get_bits(voltage,0,100,8);
    status_packet.data.temperature=(uint8_t)get_bits(stm32l0_temp,-20,80,8);
    status_packet.data.vbus=(uint8_t)get_bits(stm32l0_vdd,0,3.6,8);

    #ifdef debug
            serial_debug.print("status_send( ");
            serial_debug.print("resetCause: ");
            serial_debug.print(STM32L0.resetCause(),HEX);
            serial_debug.print(", battery: ");
            serial_debug.print(voltage);
            serial_debug.print(" ");
            serial_debug.print(status_packet.data.battery);
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

    lorawan_send(status_packet_port, &status_packet.bytes[0], sizeof(statusData_t));
}