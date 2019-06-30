function Encoder(object, port) {
    var bytes = [];
    //settings
    if (port === 3){
        bytes[0] = (object.system_status_interval) & 0xFF;
        bytes[1] = (object.system_status_interval)>>8 & 0xFF;

        bytes[2] |= object.system_functions.accelerometer_enabled  ? 1<<3 : 0;
        bytes[2] |= object.system_functions.light_enabled  ? 1<<4 : 0;
        bytes[2] |= object.system_functions.temperature_enabled  ? 1<<5 : 0;
        bytes[2] |= object.system_functions.humidity_enabled  ? 1<<6 : 0;
        bytes[2] |= object.system_functions.pressure_enabled  ? 1<<7 : 0;

        bytes[3] |= (object.lorawan_datarate_adr.datarate) & 0x0F;
        bytes[3] |= object.lorawan_datarate_adr.confirmed_uplink ? 1<<6 : 0;
        bytes[3] |= object.lorawan_datarate_adr.adr ? 1<<7 : 0;

        bytes[4] = (object.gps_periodic_interval) & 0xFF;
        bytes[5] = (object.gps_periodic_interval)>>8 & 0xFF;

        bytes[6] = (object.gps_triggered_interval) & 0xFF;
        bytes[7] = (object.gps_triggered_interval)>>8 & 0xFF;

        bytes[8] = (object.gps_triggered_threshold) & 0xFF;

        bytes[9] = (object.gps_triggered_duration) & 0xFF;

        bytes[10] = (object.gps_cold_fix_timeout) & 0xFF;
        bytes[11] = (object.gps_cold_fix_timeout)>>8 & 0xFF;

        bytes[12] = (object.gps_hot_fix_timeout) & 0xFF;
        bytes[13] = (object.gps_hot_fix_timeout)>>8 & 0xFF;

        bytes[14] = (object.gps_min_fix_time) & 0xFF;

        bytes[15] = (object.gps_min_ehpe) & 0xFF;

        bytes[16] = (object.gps_hot_fix_retry) & 0xFF;

        bytes[17] = (object.gps_cold_fix_retry) & 0xFF;
        
        bytes[18] = (object.gps_fail_retry) & 0xFF;

        bytes[19] = object.gps_settings.d3_fix ? 1<<0 : 0;
        bytes[19] |= object.gps_settings.fail_backoff ? 1<<1 : 0;
        bytes[19] |= object.gps_settings.hot_fix ? 1<<2 : 0;
        bytes[19] |= object.gps_settings.fully_resolved ? 1<<3 : 0;
    }
    //command
    else if (port === 99){
        if(object.command.reset){
            bytes[0]=0xab;
        }
        else if(object.command.lora_rejoin){
            bytes[0]=0xde;
        }
        else if(object.command.send_settings){
            bytes[0]=0xaa;
        }
    }
    return bytes;
  }
