import json

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
lorawan_datarate_adr["datarate"]=5
lorawan_datarate_adr["confirmed_uplink"]=False
lorawan_datarate_adr["adr"]=False
data['lorawan_datarate_adr'] = lorawan_datarate_adr

data['gps_periodic_interval'] =1
data['gps_triggered_interval'] =0
data['gps_triggered_threshold'] =10
data['gps_triggered_duration'] =10
data['gps_cold_fix_timeout'] =200
data['gps_hot_fix_timeout'] =60
data['gps_min_fix_time'] =1
data['gps_min_ehpe'] =50
data['gps_hot_fix_retry'] = 5
data['gps_cold_fix_retry'] =20
data['gps_fail_retry'] =0

gps_settings = {}
gps_settings['d3_fix'] = True
gps_settings['fail_backoff'] = False
gps_settings['hot_fix'] = True
gps_settings['fully_resolved'] = False

data['gps_settings'] = gps_settings
data['system_voltage_interval'] =1
data['gps_charge_min'] =2500
data['system_charge_min'] =3450
data['system_charge_max'] =4000
data['system_input_charge_min'] =10000

data['pulse_threshold'] =3
data['pulse_on_timeout'] =60
data['pulse_min_interval'] =1

data['gps_accel_z_threshold'] =0


json_data = json.dumps(data)
print(json_data)
