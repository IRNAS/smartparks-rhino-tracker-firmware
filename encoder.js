function Encoder(object, port) {
    var bytes = [];
    //settings
    if (port === 3){
        bytes[0] = (object.system_status_interval) & 0xFF;
        bytes[1] = (object.system_status_interval)>>8 & 0xFF;

        bytes[2] |= object.system_functions.gps_periodic ? 1<<0 : 0;
        bytes[2] |= object.system_functions.gps_triggered  ? 1<<1 : 0;
        bytes[2] |= object.system_functions.gps_hot_fix  ? 1<<2 : 0;
        bytes[2] |= object.system_functions.accelerometer_enabled  ? 1<<3 : 0;
        bytes[2] |= object.system_functions.light_enabled  ? 1<<4 : 0;
        bytes[2] |= object.system_functions.temperature_enabled  ? 1<<5 : 0;
        bytes[2] |= object.system_functions.humidity_enabled  ? 1<<6 : 0;
        bytes[2] |= object.system_functions.pressure_enabled  ? 1<<7 : 0;

        bytes[3] |= (object.lorawan_datarate_adr.datarate) & 0x0F;
        bytes[3] |= object.lorawan_datarate_adr.confirmed_uplink ? 1<<6 : 0;
        bytes[3] |= object.lorawan_datarate_adr.adr ? 1<<7 : 0;

        bytes[4] = (object.sensor_interval) & 0xFF;
        bytes[5] = (object.sensor_interval)>>8 & 0xFF;

        bytes[6] = (object.gps_cold_fix_timeout) & 0xFF;
        bytes[7] = (object.gps_cold_fix_timeout)>>8 & 0xFF;

        bytes[8] = (object.gps_hot_fix_timeout) & 0xFF;
        bytes[9] = (object.gps_hot_fix_timeout)>>8 & 0xFF;

        bytes[10] = (object.gps_minimal_ehpe) & 0xFF;

        bytes[11] = (object.mode_slow_voltage_threshold) & 0xFF;

        bytes[12] = object.gps_settings.d3_fix ? 1<<0 : 0;
        bytes[12] |= object.gps_settings.fail_backoff ? 1<<1 : 0;

        bytes[13] = (object.sensor_interval_active_threshold) & 0xFF;

        bytes[14] = (object.sensor_active_interval) & 0xFF;
        bytes[15] = (object.sensor_active_interval)>>8 & 0xFF;
    }
    //command
    else if (port === 99){
        bytes[0] = object.command.reset? 0xab : 0x00;
    }
    return bytes;
  }
