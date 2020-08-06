import json

data = {}
data['freq_start'] = 840000000
data['freq_stop'] = 880000000
data['samples'] = 40
data['power'] = 5
data['time'] = 100
data['type'] = 2


json_data = json.dumps(data)
print(json_data)