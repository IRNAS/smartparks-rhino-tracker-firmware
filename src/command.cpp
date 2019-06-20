#include "command.h"

uint8_t command_get_packet_port(void){
    return command_packet_port;
}

void command_receive(uint8_t command){
    if(command==0xab){
        //reset received
        STM32L0.reset();
    }
    else if(command==0xde){
        for (int i = 0 ; i < EEPROM.length() ; i++) {
            EEPROM.write(i, 0);
        }
        STM32L0.reset();
    }
    else if(command==0xaa){
        boolean settings_send_flag = true;
    }
}