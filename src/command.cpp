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
    else if(command==0xac){
        uint8_t eeprom_settings_address = EEPROM_DATA_START_SETTINGS;
        EEPROM.write(eeprom_settings_address,0x00);
        for(int i=0;i<sizeof(settingsData_t);i++){
            EEPROM.write(eeprom_settings_address+1+i,0x00);
        }
        //reset received
        STM32L0.reset();
    }
}