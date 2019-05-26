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
        decoded.system_functions = bytes[2];
        decoded.lorawan_datarate_adr = bytes[3];
        decoded.sensor_interval = (bytes[5] << 8) | bytes[4];
        decoded.gps_cold_fix_timeout = (bytes[7] << 8) | bytes[6];
        decoded.gps_hot_fix_timeout = (bytes[9] << 8) | bytes[8];
        decoded.gps_minimal_hdop = bytes[10];
        decoded.mode_slow_voltage_threshold = bytes[11];
    }
    else if (port === 2){
        decoded.resetCause = resetCause_dict[bytes[0]];
        decoded.mode = bytes[1];
        decoded.battery = get_num(bytes[2],0,100,8,1);
        decoded.temperature = get_num(bytes[3],-20,80,8,1);
        decoded.vbus = get_num(bytes[4],0,3.6,8,2);
        decoded.system_functions_errors = bytes[5];
      }
      else if (port === 1){
        decoded.status = bytes[0];
      }

    return decoded;
  }