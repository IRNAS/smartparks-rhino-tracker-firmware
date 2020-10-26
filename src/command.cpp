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
        // due to a bug is sesion clearing, OTAA must be forced and then device reset
        const char *appEui  = "0101010101010101";
        const char *appKey  = "2B7E151628AED2A6ABF7158809CF4F3C";
        const char *devEui  = "0101010101010101";
        LoRaWAN.setSaveSession(false);
        LoRaWAN.joinOTAA(appEui, appKey, devEui);
        delay(1000);
        STM32L0.reset();
    }
    else if(command==0xaa){
        settings_send_flag = true;
    }
    else if(command==0x11){
        gps_log_flag = true;
    }
    else if(command==0xf1){
        gps_log_clear();
    }
    else if(command==0xd0){
        status_dropoff_flag=true;
    }    
}