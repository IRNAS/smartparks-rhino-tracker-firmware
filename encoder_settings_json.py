import json

data = {}
data['system_status_interval'] = 10

system_functions = {}
system_functions['gps_periodic'] = True
system_functions['gps_triggered'] = False
system_functions['gps_hot_fix'] = True
system_functions['accelerometer_enabled'] = False
system_functions['light_enabled'] = False
system_functions['temperature_enabled'] = True
system_functions['humidity_enabled'] = True
system_functions['pressure_enabled'] = True
data['system_functions'] = system_functions

lorawan_datarate_adr = {}
lorawan_datarate_adr["datarate"]=3
lorawan_datarate_adr["confirmed_uplink"]=False
lorawan_datarate_adr["adr"]=False
data['lorawan_datarate_adr'] = lorawan_datarate_adr

data['sensor_interval'] = 60

data['gps_cold_fix_timeout'] = 180

data['gps_hot_fix_timeout'] = 180

data['gps_minimal_ehpe'] = 50

data['mode_slow_voltage_threshold'] = 1

gps_settings = {}
gps_settings['d3_fix'] = True
data['gps_settings'] = gps_settings

json_data = json.dumps(data)
print(json_data)