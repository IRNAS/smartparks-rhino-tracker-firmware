import json

data = {}
data['freq_start'] = 800000000
data['freq_stop'] = 900000000
data['samples'] = 20
data['power'] = 14
data['time'] = 100
data['type'] = 0


json_data = json.dumps(data)
print(json_data)