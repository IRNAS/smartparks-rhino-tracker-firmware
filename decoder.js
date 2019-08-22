function get_num(x, min, max, precision, round) {
  
	var range = max - min;
	var new_range = (Math.pow(2, precision) - 1) / range;
	var back_x = x / new_range;
	
	if (back_x===0) {
		back_x = min;
	}
	else if (back_x === (max - min)) {
		back_x = max;
	}
	else {
		back_x += min;
	}
	return Math.round(back_x*Math.pow(10,round))/Math.pow(10,round);
}

function Decoder(bytes) {

    var decoded = {};

    var resetCause_dict = {
        0:"POWERON",
        1:"EXTERNAL",
        2:"SOFTWARE",
        3:"WATCHDOG",
        4:"FIREWALL",
        5:"OTHER",
        6:"STANDBY"
    };

    // settings
    if (port === 3){
        decoded.system_status_interval = (bytes[1] << 8) | bytes[0];
        decoded.system_functions = {};//bytes[2];
        decoded.system_functions.gps_periodic =  ((bytes[2] >> 0)&0x01)? 1 : 0;
        decoded.system_functions.gps_triggered =  ((bytes[2] >> 1)&0x01)? 1 : 0;
        decoded.system_functions.gps_hot_fix =  ((bytes[2] >> 2)&0x01)? 1 : 0;
        decoded.system_functions.accelerometer_enabled =  ((bytes[2] >> 3)&0x01)? 1 : 0;
        decoded.system_functions.light_enabled =  ((bytes[2] >> 4)&0x01)? 1 : 0;
        decoded.system_functions.temperature_enabled =  ((bytes[2] >> 5)&0x01)? 1 : 0;
        decoded.system_functions.humidity_enabled =  ((bytes[2] >> 6)&0x01)? 1 : 0;
        decoded.system_functions.pressure_enabled =  ((bytes[2] >> 7)&0x01)? 1 : 0;

        decoded.lorawan_datarate_adr = {};//bytes[3];
        decoded.lorawan_datarate_adr.datarate = bytes[3]&0x0f;
        decoded.lorawan_datarate_adr.confirmed_uplink =  ((bytes[3] >> 6)&0x01)? 1 : 0;
        decoded.lorawan_datarate_adr.adr =  ((bytes[3] >> 7)&0x01)? 1 : 0;

        decoded.gps_periodic_interval = (bytes[5] << 8) | bytes[4];
        decoded.gps_triggered_interval = (bytes[7] << 8) | bytes[6];
        decoded.gps_triggered_threshold = bytes[8];
        decoded.gps_triggered_duration = bytes[9];
        decoded.gps_cold_fix_timeout = (bytes[11] << 8) | bytes[10];
        decoded.gps_hot_fix_timeout = (bytes[13] << 8) | bytes[12];
        decoded.gps_min_fix_time = bytes[14];
        decoded.gps_min_ehpe = bytes[15];
        decoded.gps_hot_fix_retry = bytes[16];
        decoded.gps_cold_fix_retry = bytes[17];
        decoded.gps_fail_retry = bytes[18];
        decoded.gps_settings = {};//bytes[19];
        decoded.gps_settings.d3_fix =  ((bytes[19] >> 0)&0x01)? 1 : 0;
        decoded.gps_settings.fail_backoff =  ((bytes[19] >> 1)&0x01)? 1 : 0;
        decoded.gps_settings.hot_fix =  ((bytes[19] >> 2)&0x01)? 1 : 0;
        decoded.gps_settings.fully_resolved =  ((bytes[19] >> 3)&0x01)? 1 : 0;
    }
    else if (port === 2){
        decoded.resetCause = resetCause_dict[bytes[0]];
        decoded.battery_low = get_num(bytes[1],400,4000,8,1);
        decoded.battery = get_num(bytes[2],400,4000,8,1);
        decoded.temperature = get_num(bytes[3],-20,80,8,1);
        decoded.vbus = get_num(bytes[4],0,3.6,8,2);
        decoded.system_functions_errors = {};//bytes[5];
        decoded.system_functions_errors.gps_periodic_error =  ((bytes[5] >> 0)&0x01)? 1 : 0;
        decoded.system_functions_errors.gps_triggered_error =  ((bytes[5] >> 1)&0x01)? 1 : 0;
        decoded.system_functions_errors.gps_fix_error =  ((bytes[5] >> 2)&0x01)? 1 : 0;
        decoded.system_functions_errors.accelerometer_error =  ((bytes[5] >> 3)&0x01)? 1 : 0;
        decoded.system_functions_errors.light_error =  ((bytes[5] >> 4)&0x01)? 1 : 0;
        decoded.system_functions_errors.temperature_error =  ((bytes[5] >> 5)&0x01)? 1 : 0;
        decoded.system_functions_errors.humidity_error =  ((bytes[5] >> 6)&0x01)? 1 : 0;
        decoded.system_functions_errors.pressure_error =  ((bytes[5] >> 7)&0x01)? 1 : 0;
      }
    else if (port === 1){
        decoded.lat = ((bytes[0]<<16)>>>0) + ((bytes[1]<<8)>>>0) + bytes[2];
        decoded.lat = (decoded.lat / 16777215.0 * 180) - 90;
        decoded.lon = ((bytes[3]<<16)>>>0) + ((bytes[4]<<8)>>>0) + bytes[5];
        decoded.lon = (decoded.lon / 16777215.0 * 360) - 180;
        decoded.alt = (bytes[7] << 8) | bytes[6];
        decoded.satellites = (bytes[8] >> 4);
        decoded.hdop = (bytes[8] & 0x0f);
        decoded.time_to_fix = bytes[9];
        decoded.epe = bytes[10];
        decoded.snr = bytes[11];
        decoded.lux = bytes[12];
        decoded.motion = bytes[13];
      }

    return decoded;
  }