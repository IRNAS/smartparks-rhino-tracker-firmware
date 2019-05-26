#include "command.h"

uint8_t command_get_packet_port(void){
    return command_packet_port;
}

void command_receive(uint8_t command){
    if(command==0xab){
        //reset received
        STM32L0.reset();
    }
}