import json

data = {}
data['system_status_interval'] = 5

system_functions = {}
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

data['gps_periodic_interval'] =60
data['gps_triggered_interval'] =0
data['gps_triggered_threshold'] =0
data['gps_triggered_duration'] =0
data['gps_cold_fix_timeout'] =200
data['gps_hot_fix_timeout'] =30
data['gps_min_fix_time'] =5
data['gps_min_ehpe'] =50
data['gps_hot_fix_retry'] = 10
data['gps_cold_fix_retry'] =3
data['gps_fail_retry'] =1

gps_settings = {}
gps_settings['d3_fix'] = True
gps_settings['fail_backoff'] = False
gps_settings['hot_fix'] = True

data['gps_settings'] = gps_settings

json_data = json.dumps(data)
print(json_data)