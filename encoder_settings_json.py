import json
import struct
import argparse

"""Set values"""
data = {}
data['system_status_interval'] = 5

system_functions = {}
system_functions['accelerometer_enabled'] = False
system_functions['light_enabled'] = False
system_functions['temperature_enabled'] = False
system_functions['humidity_enabled'] = False
system_functions['charging_enabled'] = True
data['system_functions'] = system_functions

lorawan_datarate_adr = {}
lorawan_datarate_adr["datarate"] = 5
lorawan_datarate_adr["confirmed_uplink"] = False
lorawan_datarate_adr["adr"] = False
data['lorawan_datarate_adr'] = lorawan_datarate_adr

data['gps_periodic_interval'] = 1
data['gps_triggered_interval'] = 0
data['gps_triggered_threshold'] = 10
data['gps_triggered_duration'] = 10
data['gps_cold_fix_timeout'] = 200
data['gps_hot_fix_timeout'] = 60
data['gps_min_fix_time'] = 1
data['gps_min_ehpe'] = 50
data['gps_hot_fix_retry'] = 5
data['gps_cold_fix_retry'] = 20
data['gps_fail_retry'] = 0

gps_settings = {}
gps_settings['d3_fix'] = True
gps_settings['fail_backoff'] = False
gps_settings['hot_fix'] = True
gps_settings['fully_resolved'] = False

data['gps_settings'] = gps_settings
data['system_voltage_interval'] = 1
data['gps_charge_min'] = 2500
data['system_charge_min'] = 3450
data['system_charge_max'] = 4000
data['system_input_charge_min'] = 10000

data['pulse_threshold'] = 3
data['pulse_on_timeout'] = 60
data['pulse_min_interval'] = 1

data['gps_accel_z_threshold'] = 0

json_data = json.dumps(data)
print(f"Settings json object: {json_data}")

"""Use helper variables to correctly encode bits"""
system_status_setting = data['system_status_interval']  # 2 bytes in size

# 1 byte in size
system_functions_byte = 0 | \
    system_functions['accelerometer_enabled'] << 3 | \
    system_functions['light_enabled'] << 4 | \
    system_functions['temperature_enabled'] << 5 | \
    system_functions['humidity_enabled'] << 6 | \
    system_functions['charging_enabled'] << 7

# 1 byte in size
lorawan_datarate_adr_byte = lorawan_datarate_adr['datarate'] & 0x0F | \
    lorawan_datarate_adr['confirmed_uplink'] << 6 | \
    lorawan_datarate_adr['adr'] << 7

# 2 bytes gps_periodic_interval
# 2 bytes gps_triggered_interval
# 1 byte gps triggered_threshold
# 1 byte gps_triggerd_duration
# 2 bytes gps_cold_fix_timeout
# 2 bytes gps_hot_fix_timeout
# 1 byte gps_min_fix_time
# 1 byte gps_min_ehpe
# 1 byte gps_hot_fix_retry
# 1 byte gps_cold_fix_retry
# 1 byte gps_fail_retry

# 1 byte gps_settings
gps_settings_byte = gps_settings['d3_fix'] | \
    gps_settings['fail_backoff'] << 1 | \
    gps_settings['hot_fix'] << 2 | \
    gps_settings['fully_resolved'] << 3

# 1 byte system_voltage_interval

# 1 byte gps_charge min, convert to correct unit
gps_charge_min = int((data['gps_charge_min'] - 2500) / 10) & 0xFF
# 1 byte system_charge_min
system_charge_min = int((data['system_charge_min'] - 2500) / 10) & 0xFF
# 1 byte system_charge_max
system_charge_max = int((data['system_charge_max'] - 2500) / 10) & 0xFF

# 2 bytes system input charge min
# 1 byte pulse threshold
# 1 byte pulse_on_timeout
# 2 bytes pulse_min_interval

# 2 bytes gps_accel_z_threshold
gps_accel_z_threshold = data['gps_accel_z_threshold'] + 2000

# 2 zero bytes
print(f"System functions byte: {system_functions_byte}")

binary = struct.pack('<HBBHHBBHHBBBBBBBBBBHBBHHBB',
                     # H
                     system_status_setting,
                     # B
                     system_functions_byte,
                     # B
                     lorawan_datarate_adr_byte,
                     # H
                     data['gps_periodic_interval'],
                     # H
                     data['gps_triggered_interval'],
                     # B
                     data['gps_triggered_threshold'],
                     # B
                     data['gps_triggered_duration'],
                     # H
                     data['gps_cold_fix_timeout'],
                     # H
                     data['gps_hot_fix_timeout'],
                     # B
                     data['gps_min_fix_time'],
                     # B
                     data['gps_min_ehpe'],
                     # B
                     data['gps_hot_fix_retry'],
                     # B
                     data['gps_cold_fix_retry'],
                     # B
                     data['gps_fail_retry'],
                     # B
                     gps_settings_byte,
                     # B
                     data['system_voltage_interval'],
                     # B
                     gps_charge_min,
                     # B
                     system_charge_min,
                     # B
                     system_charge_max,
                     # H
                     data['system_input_charge_min'],
                     # B
                     data['pulse_threshold'],
                     # B
                     data['pulse_on_timeout'],
                     # H
                     data['pulse_min_interval'],
                     # H
                     gps_accel_z_threshold,
                     # B
                     0,
                     # B
                     0
                     )

# Initialize argparse
parser = argparse.ArgumentParser(description='specify if test of payload is required or not')
parser.add_argument("-t", "--test", action="store_true", help="Specifiy wether to test set payload")
parser.set_defaults(test=False)
args = parser.parse_args()

js_encoder_output = []

if args.test:
    import subprocess
    import sys

    try:
        import js2py
    except ImportError:
        subprocess.check_call([sys.executable, "-m", "pip", "install", 'js2py'])
    finally:
        import js2py
        
    """Use old encoder to confirm new encoder is reliable"""
    js = """
    function Encoder(object, port) {
        var bytes = [];

        //settings
        if (port === 3) {
            bytes[0] = (object.system_status_interval) & 0xFF;
            bytes[1] = (object.system_status_interval) >> 8 & 0xFF;

            bytes[2] |= object.system_functions.accelerometer_enabled ? 1 << 3 : 0;
            bytes[2] |= object.system_functions.light_enabled ? 1 << 4 : 0;
            bytes[2] |= object.system_functions.temperature_enabled ? 1 << 5 : 0;
            bytes[2] |= object.system_functions.humidity_enabled ? 1 << 6 : 0;
            bytes[2] |= object.system_functions.charging_enabled ? 1 << 7 : 0;

            bytes[3] |= (object.lorawan_datarate_adr.datarate) & 0x0F;
            bytes[3] |= object.lorawan_datarate_adr.confirmed_uplink ? 1 << 6 : 0;
            bytes[3] |= object.lorawan_datarate_adr.adr ? 1 << 7 : 0;

            bytes[4] = (object.gps_periodic_interval) & 0xFF;
            bytes[5] = (object.gps_periodic_interval) >> 8 & 0xFF;

            bytes[6] = (object.gps_triggered_interval) & 0xFF;
            bytes[7] = (object.gps_triggered_interval) >> 8 & 0xFF;

            bytes[8] = (object.gps_triggered_threshold) & 0xFF;

            bytes[9] = (object.gps_triggered_duration) & 0xFF;

            bytes[10] = (object.gps_cold_fix_timeout) & 0xFF;
            bytes[11] = (object.gps_cold_fix_timeout) >> 8 & 0xFF;

            bytes[12] = (object.gps_hot_fix_timeout) & 0xFF;
            bytes[13] = (object.gps_hot_fix_timeout) >> 8 & 0xFF;

            bytes[14] = (object.gps_min_fix_time) & 0xFF;

            bytes[15] = (object.gps_min_ehpe) & 0xFF;

            bytes[16] = (object.gps_hot_fix_retry) & 0xFF;

            bytes[17] = (object.gps_cold_fix_retry) & 0xFF;

            bytes[18] = (object.gps_fail_retry) & 0xFF;

            bytes[19] = object.gps_settings.d3_fix ? 1 << 0 : 0;
            bytes[19] |= object.gps_settings.fail_backoff ? 1 << 1 : 0;
            bytes[19] |= object.gps_settings.hot_fix ? 1 << 2 : 0;
            bytes[19] |= object.gps_settings.fully_resolved ? 1 << 3 : 0;
            bytes[20] = (object.system_voltage_interval) & 0xFF;
            bytes[21] = ((object.gps_charge_min - 2500) / 10) & 0xFF;
            bytes[22] = ((object.system_charge_min - 2500) / 10) & 0xFF;
            bytes[23] = ((object.system_charge_max - 2500) / 10) & 0xFF;
            bytes[24] = (object.system_input_charge_min) & 0xFF;
            bytes[25] = (object.system_input_charge_min) >> 8 & 0xFF;
            bytes[26] = (object.pulse_threshold) & 0xFF;
            bytes[27] = (object.pulse_on_timeout) & 0xFF

            bytes[28] = (object.pulse_min_interval) & 0xFF;
            bytes[29] = (object.pulse_min_interval) >> 8 & 0xFF;

            bytes[30] = (object.gps_accel_z_threshold + 2000) & 0xFF;
            bytes[31] = (object.gps_accel_z_threshold + 2000) >> 8 & 0xFF;

            bytes[32] = 0;
            bytes[33] = 0;
        }
        return bytes;
    }
    Encoder(python_json_string_input, python_input_port);
    """

    js_encoder_output = js2py.eval_js6(js.replace(
        "python_json_string_input", json_data).replace("python_input_port", "3"))

if args.test:
    if list(binary) == list(js_encoder_output):
        print(f"Encoded payload: {binary.hex()}")
    else:
        print(f"Encoded payload {list(binary)} does not match encoded payload from previous version of the encoder: {list(js_encoder_output)}")