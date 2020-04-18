import json

data = {}
data['freq_start'] = 830000000
data['freq_stop'] = 900000000
data['samples'] = 70
data['power'] = 10
data['time'] = 1000
data['type'] = 0


json_data = json.dumps(data)
print(json_data)