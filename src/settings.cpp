#include "settings.h"

//#define debug
//#define serial_debug  Serial
//#define FORCE_DEFAULT_SETTINGS // TODO: remove for production

boolean settings_updated = false;
settingsPacket_t settings_packet;
settingsPacket_t settings_packet_downlink;
calibrationPacket_t calibration_packet;

/**
 * @brief Setttings get lorawan settings port
 * 
 * @return uint8_t port
 */
uint8_t settings_get_packet_port(void){
    return settings_packet_port;
}

/**
 * @brief Settings init - read and configure settings upon boot or update
 * 
 * @details The settings are read from eeprom if implemented or hardcoded defaults are used here. See .h file for packet contents description.
 * 
 */
void settings_init(void){
    //default settings
    settings_packet.data.system_status_interval=10;
    settings_packet.data.system_functions=0xff;
    settings_packet.data.lorawan_datarate_adr=3;
    settings_packet.data.gps_periodic_interval=5;
    settings_packet.data.gps_triggered_interval=0;
    settings_packet.data.gps_triggered_threshold=0x0f;
    settings_packet.data.gps_triggered_duration=0x0f;
    settings_packet.data.gps_cold_fix_timeout=60;
    settings_packet.data.gps_hot_fix_timeout=10;
    settings_packet.data.gps_min_fix_time=5;
    settings_packet.data.gps_min_ehpe=50;
    settings_packet.data.gps_hot_fix_retry=5;
    settings_packet.data.gps_cold_fix_retry=2;
    settings_packet.data.gps_fail_retry=1;
    settings_packet.data.gps_settings=0b10001101;
    settings_packet.data.system_voltage_interval=30;
    settings_packet.data.gps_charge_min=0;
    settings_packet.data.system_charge_min=0;
    settings_packet.data.system_charge_max=255;
    settings_packet.data.system_input_charge_min=10000;
    settings_packet.data.pulse_threshold=0;
    settings_packet.data.pulse_on_timeout=0;
    settings_packet.data.pulse_min_interval=0;
    settings_packet.data.gps_accel_z_threshold=2000;

    //check if valid settings present in eeprom 
    //uint8_t EEPROM_DATA_START_SETTINGS = EEPROM_DATA_START_SETTINGS;
    #ifndef FORCE_DEFAULT_SETTINGS
    if(EEPROM.read(EEPROM_DATA_START_SETTINGS)==0xab){
        for(int i=0;i<sizeof(settingsData_t);i++){
            settings_packet.bytes[i]=EEPROM.read(EEPROM_DATA_START_SETTINGS+8+i);
        }
    }
    #endif
    // reading here as this must not be restored from flash
    settings_packet.data.fw_version=FW_VERSION;

    // note calibration data is 8 bytes inclusive of settings check byte
    for(int i=0;i<sizeof(calibrationData_t);i++){
        calibration_packet.bytes[i]=EEPROM.read(EEPROM_DATA_START_SETTINGS+i);
    }

    // check if the calibration value is not set
    if(calibration_packet.data.ads_calib ==0){
        calibration_packet.data.ads_calib = 1;
    }

    // TEST METHOD TO ENTER SETTINGS MANUALLY
    //calibration_packet.data.ads_calib = 1.8575; // test-1
    //calibration_packet.data.ads_calib = 1.0; // test-2
    //calibration_packet.data.ads_calib = 3.084; // test-3
    //calibration_packet.data.ads_calib = 1.2000; // test-5
    //calibration_packet.data.ads_calib = 1.0; // test-6
    //calibration_packet.data.ads_calib = 1.1711; // test-7

    // for(int i=0;i<sizeof(calibrationData_t);i++){
    //     EEPROM.write(EEPROM_DATA_START_SETTINGS+i,calibration_packet.bytes[i]);
    // }
    
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
    settings_packet.data.system_functions=constrain(settings_packet_downlink.data.system_functions, 0,0xff);
    settings_packet.data.lorawan_datarate_adr=constrain(settings_packet_downlink.data.lorawan_datarate_adr, 0, 0xff);
    settings_packet.data.gps_periodic_interval=constrain(settings_packet_downlink.data.gps_periodic_interval, 0, 24*60);
    settings_packet.data.gps_triggered_interval=constrain(settings_packet_downlink.data.gps_triggered_interval, 0, 24*60);
    settings_packet.data.gps_triggered_threshold=constrain(settings_packet_downlink.data.gps_triggered_threshold, 0,0x3f);
    settings_packet.data.gps_triggered_duration=constrain(settings_packet_downlink.data.gps_triggered_duration, 0,0xff);
    settings_packet.data.gps_cold_fix_timeout=constrain(settings_packet_downlink.data.gps_cold_fix_timeout, 0,600);
    settings_packet.data.gps_hot_fix_timeout=constrain(settings_packet_downlink.data.gps_hot_fix_timeout, 0,600);
    settings_packet.data.gps_min_fix_time=constrain(settings_packet_downlink.data.gps_min_fix_time, 0,60);
    settings_packet.data.gps_min_ehpe=constrain(settings_packet_downlink.data.gps_min_ehpe, 0,100);
    settings_packet.data.gps_hot_fix_retry=constrain(settings_packet_downlink.data.gps_hot_fix_retry, 0,0xff);
    settings_packet.data.gps_cold_fix_retry=constrain(settings_packet_downlink.data.gps_cold_fix_retry, 0,0xff);
    settings_packet.data.gps_fail_retry=constrain(settings_packet_downlink.data.gps_fail_retry, 0,0xff); //must be 1 due to bug in GPS core
    settings_packet.data.gps_settings=constrain(settings_packet_downlink.data.gps_settings, 0,0xff);
    settings_packet.data.system_voltage_interval=constrain(settings_packet_downlink.data.system_voltage_interval, 0,0xff);
    settings_packet.data.gps_charge_min=constrain(settings_packet_downlink.data.gps_charge_min, 0,0xff);
    settings_packet.data.system_charge_min=constrain(settings_packet_downlink.data.system_charge_min, 0,0xff);
    settings_packet.data.system_charge_max=constrain(settings_packet_downlink.data.system_charge_max, 0,0xff);
    settings_packet.data.system_input_charge_min=constrain(settings_packet_downlink.data.system_input_charge_min, 0,0xffff);
    settings_packet.data.pulse_on_timeout=constrain(settings_packet_downlink.data.pulse_on_timeout, 0,0xff);
    settings_packet.data.pulse_threshold=constrain(settings_packet_downlink.data.pulse_threshold, 0,0xff);
    settings_packet.data.pulse_min_interval=constrain(settings_packet_downlink.data.pulse_min_interval, 0,0xffff);
    settings_packet.data.gps_accel_z_threshold=constrain(settings_packet_downlink.data.gps_accel_z_threshold, 0,0xffff);

    // Checks against stupid configurations

    // Hot-fix timeout should not be smaller then the cold-fix timeout
    if(settings_packet.data.gps_hot_fix_timeout>settings_packet.data.gps_cold_fix_timeout){
        settings_packet.data.gps_hot_fix_timeout=settings_packet.data.gps_cold_fix_timeout;
    }

    // Min fix time should not be greater the hot fix time
    if(settings_packet.data.gps_min_fix_time>settings_packet.data.gps_hot_fix_timeout){
        settings_packet.data.gps_min_fix_time=settings_packet.data.gps_hot_fix_timeout;
    }

    EEPROM.write(EEPROM_DATA_START_SETTINGS,0xab);
    for(int i=0;i<sizeof(settingsData_t);i++){
        EEPROM.write(EEPROM_DATA_START_SETTINGS+8+i,settings_packet.bytes[i]);
    }
    //EEPROM.put(EEPROM_DATA_START_SETTINGS,settings_packet.bytes); // does not work on the byte array
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