#include "settings.h"

//#define debug
//#define serial_debug  Serial

boolean settings_send_flag = false;
boolean settings_updated = false;
settingsPacket_t settings_packet;
settingsPacket_t settings_packet_downlink;

/**
 * @brief Setttings get lorawan settings port
 * 
 * @return uint8_t port
 */
uint8_t settings_get_packet_port(void){
    return settings_packet_port;
}

/**
 * @brief Settings timer callback - generally not used
 * 
 */
void settings_timer_callback(void)
{
  #ifdef debug
    serial_debug.println("settings_timer_callback()");
  #endif
  settings_send_flag = true;
}

/**
 * @brief Settings init - read and configure settings upon boot or update
 * 
 * @details The settings are read from eeprom if implemented or hardcoded defaults are used here. See .h file for packet contents description.
 * 
 */
void settings_init(void){
    //hardcoded defaults
    settings_packet.data.system_status_interval=5;
    settings_packet.data.system_functions=0xff;
    settings_packet.data.lorawan_datarate_adr=3;
    settings_packet.data.sensor_interval=2;
    settings_packet.data.gps_cold_fix_timeout=200;
    settings_packet.data.gps_hot_fix_timeout=200;
    settings_packet.data.gps_minimal_ehpe=50;
    settings_packet.data.mode_slow_voltage_threshold=1;
    settings_packet.data.gps_settings=0x00;

    // here implement reading from eeprom

    #ifdef debug
        serial_debug.println("lorawan_load_settings()");
    #endif
}

/**
 * @brief Settings from downlink parses the binary lorawan packet and performs range validation on all values and applies them. Sotres to eeprom if implemented.
 * 
 */
void settings_from_downlink(void)
{
    // perform validation
    // copy to main settings
    settings_packet.data.system_status_interval=constrain(settings_packet_downlink.data.system_status_interval, 1, 24*60);
    settings_packet.data.system_functions=constrain(settings_packet_downlink.data.system_functions, 0, 0xff);
    settings_packet.data.lorawan_datarate_adr=constrain(settings_packet_downlink.data.lorawan_datarate_adr, 0, 5);
    settings_packet.data.sensor_interval=constrain(settings_packet_downlink.data.sensor_interval, 1, 24*60);
    settings_packet.data.gps_cold_fix_timeout=constrain(settings_packet_downlink.data.gps_cold_fix_timeout, 0, 600);
    settings_packet.data.gps_hot_fix_timeout=constrain(settings_packet_downlink.data.gps_hot_fix_timeout, 0, 600);
    settings_packet.data.gps_minimal_ehpe=constrain(settings_packet_downlink.data.gps_minimal_ehpe, 0, 100);
    settings_packet.data.mode_slow_voltage_threshold=constrain(settings_packet_downlink.data.mode_slow_voltage_threshold, 1, 100);
    settings_packet.data.gps_settings=constrain(settings_packet_downlink.data.gps_settings, 0, 0xff);
    //store to eeprom when implemented

    settings_updated = true;
}

/**
 * @brief Settings send via lorawan
 * 
 */
boolean settings_send(void){
    #ifdef debug
        serial_debug.println("settings_send()");
    #endif
    return lorawan_send(settings_packet_port, &settings_packet.bytes[0], sizeof(settingsData_t));
}