import json

data = {}
data['system_status_interval'] = 5

system_functions = {}
system_functions['gps_periodic'] = True
system_functions['gps_triggered'] = False
system_functions['gps_hot_fix'] = True
system_functions['accelerometer_enabled'] = False
system_functions['light_enabled'] = False
system_functions['temperature_enabled'] = False
system_functions['humidity_enabled'] = False
system_functions['pressure_enabled'] = False
data['system_functions'] = system_functions

lorawan_datarate_adr = {}
lorawan_datarate_adr["datarate"]=3
lorawan_datarate_adr["confirmed_uplink"]=False
lorawan_datarate_adr["adr"]=False
data['lorawan_datarate_adr'] = lorawan_datarate_adr

data['sensor_interval'] =1

data['gps_cold_fix_timeout'] = 300

data['gps_hot_fix_timeout'] = 30

data['gps_minimal_ehpe'] = 40

data['mode_slow_voltage_threshold'] = 1

gps_settings = {}
gps_settings['d3_fix'] = True
gps_settings['fail_backoff'] = True
gps_settings['minimum_fix_time'] = 2

data['gps_settings'] = gps_settings
data['sensor_interval_active_threshold'] =100
data['sensor_active_interval'] = 1

json_data = json.dumps(data)
print(json_data)