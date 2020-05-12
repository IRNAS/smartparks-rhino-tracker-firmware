import json

data = {}
data['system_status_interval'] = 30

system_functions = {}
system_functions['accelerometer_enabled'] = True
system_functions['light_enabled'] = False
system_functions['temperature_enabled'] = False
system_functions['humidity_enabled'] = False
system_functions['charging_enabled'] = True
data['system_functions'] = system_functions

lorawan_datarate_adr = {}
lorawan_datarate_adr["datarate"]=2
lorawan_datarate_adr["confirmed_uplink"]=False
lorawan_datarate_adr["adr"]=False
data['lorawan_datarate_adr'] = lorawan_datarate_adr

data['gps_periodic_interval'] = 15
data['gps_triggered_interval'] = 0
data['gps_triggered_threshold'] = 10
data['gps_triggered_duration'] = 10
data['gps_cold_fix_timeout'] = 200
data['gps_hot_fix_timeout'] = 60
data['gps_min_fix_time'] = 5
data['gps_min_ehpe'] = 50
data['gps_hot_fix_retry'] = 5
data['gps_cold_fix_retry'] = 2
data['gps_fail_retry'] = 0

gps_settings = {}
gps_settings['d3_fix'] = True
gps_settings['fail_backoff'] = False
gps_settings['hot_fix'] = True
gps_settings['fully_resolved'] = False

data['gps_settings'] = gps_settings
data['system_voltage_interval'] = 30
data['gps_charge_min'] = 3000
data['system_charge_min'] = 3900
data['system_charge_max'] = 4100
data['system_input_charge_min'] = 10000

json_data = json.dumps(data)
print(json_data)
