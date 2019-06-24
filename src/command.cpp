#include "command.h"

/**
 * @brief returns the port specified for the commands
 * 
 * @return uint8_t 
 */
uint8_t command_get_packet_port(void){
    return command_packet_port;
}

/**
 * @brief receives single byte commands via LoraWAN and performs specified action
 * 
 * @param command 
 */
void command_receive(uint8_t command){
    if(command==0xab){
        //reset received
        STM32L0.reset();
    }
    else if(command==0xde){
        LoRaWAN.setSaveSession(false);
        LoRaWAN.rejoinOTAA();
    }
    else if(command==0xaa){
        boolean settings_send_flag = true;
    }
}