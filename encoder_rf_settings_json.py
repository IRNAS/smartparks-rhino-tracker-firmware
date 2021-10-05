import json
import struct
import argparse

"""
Run with `python3 encoder_rf_settings_json.py [--test]`.
--test is an optional parameter which specifies whether to test the new version of encoder against the old js version of the encoder.
WARNING: This might slow things down substantially.
"""

"""Set values"""
data = {}
data['freq_start'] = 840000000
data['freq_stop'] = 880000000
data['samples'] = 40
data['power'] = 5
data['time'] = 100
data['type'] = 2

json_data = json.dumps(data)
print(f"Settings json object: {json_data}")

"""Use helper variables to correctly encode bits"""
binary = struct.pack('<LLLHHH', 
    # L - unsigned long
    data['freq_start'],
    # L - unsigned long
    data['freq_stop'],
    # L - unsigned long
    data['samples'],
    # H - unsigned short
    data['power'],
    # H - unsigned short
    data['time'],
    # H - unsigned short
    data['type']
)


# Initialize argparse
parser = argparse.ArgumentParser()
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
            // rf settings
            if (port === 30){
                bytes[0] = (object.freq_start) & 0xFF;
                bytes[1] = (object.freq_start)>>8 & 0xFF;
                bytes[2] = (object.freq_start)>>16 & 0xFF;
                bytes[3] = (object.freq_start)>>24 & 0xFF;

                bytes[4] = (object.freq_stop) & 0xFF;
                bytes[5] = (object.freq_stop)>>8 & 0xFF;
                bytes[6] = (object.freq_stop)>>16 & 0xFF;
                bytes[7] = (object.freq_stop)>>24 & 0xFF;

                bytes[8] = (object.samples) & 0xFF;
                bytes[9] = (object.samples)>>8 & 0xFF;
                bytes[10] = (object.samples)>>16 & 0xFF;
                bytes[11] = (object.samples)>>24 & 0xFF;

                bytes[12] = (object.power) & 0xFF;
                bytes[13] = (object.power)>>8 & 0xFF;

                bytes[14] = (object.time) & 0xFF;
                bytes[15] = (object.time)>>8 & 0xFF;

                bytes[16] = (object.type) & 0xFF;
                bytes[17] = (object.type)>>8 & 0xFF;
            }
        return bytes;
    }
    Encoder(python_json_string_input, python_input_port);
    """

    js_encoder_output = js2py.eval_js6(js.replace(
        "python_json_string_input", json_data).replace("python_input_port", "30"))

if args.test:
    if list(binary) == list(js_encoder_output):
        print(f"Encoded payload: {binary.hex()}")
    else:
        print(f"Encoded payload {list(binary)} does not match encoded payload from previous version of the encoder: {list(js_encoder_output)}")
else:
    print(f"WARNING: Payload not verified with old version of encoder!")
    print(f"Encoded payload: {binary.hex()}")