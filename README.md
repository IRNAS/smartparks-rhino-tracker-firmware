# smartparks-rhino/lion-tracker-firmware
Smartparks Rhino and Lion GPS tracker firmware repository. This firmware is the first generation solution for multiple LoraWAN devices. The implementation is as straightforward as possible to ensure simple operation, however not suitable for very complex device operation taks which is implemented int eh second generation in a separate repository.

# Requirements:
 * Modified Arduino Core for STM32L0
 * vscode or similar compile tool (Arduino IDE may break things)
 * this repository
 * TheThingsNetwork or other similar LoraWAN network server solution

# Operation
This device is a power-efficeint GPS tracker sending data via LoraWAN. There are three key messages being sent by the device:
 * `settings - port 3` - current device settings - only upon boot, change or request
 * `status - port 2 ` - (legacy, not enabled in latest version anymore) device status message being sent with the specified `status_interval` in minutes - recommended to do this every 24h
 * `gps - port 1` - gps location - gps packet is set based on the `settings` and various intervals defined there.
 * `status+gps - port 12` - device status message being sent with the specified `status_interval` in minutes - recommended to do this every 24h, combined with last gps position. Particularly useful if gps fix interval is longer then the status interval.
 * `gps log` - 

## Secrets
Put the LoRa keys in `secrets.h` file

    #define RELAY_NETWORKKEY "..."
    #define RELAY_APPKEY "..."
    #define RELAY_DEVICEADDRESS "..."

## Reed switch
The magnetic REED switch is put in place to put the device in hibernation for transport purposes or prior to installation. Removing the magnet will initialize the operation in 60s of removing it. Note that putting the device in hibernation performs a system reset and forces an OTAA rejoin if that mode is programmed.

## Pulse counting mode
The pulse counting and reporting mode is built to enable interfacing an external piece of electronics, for example a camera trap and detec actions as well as to control an ouput.

Pulse input does the following:
- Increment the field in status packet `pulse_count`
- When `pulse_count` is greater then `pulse_threshold`, send status packet and reset the field. Set to 1 to send data on every pulse or set to 0 to disable.
- The device trigger sending at most often on `pulse_min_interval` in seconds, maximum value 65535.

Pulse output does the following:
- Replicate the input pulse to the output
- When the sending conditions are met, the output pulse pin is turned on for `pulse_on_timeout` seconds.

Configuration variables controlling this:
* `pulse_threshold` - how many pulses must be received to send a status packet
* `pulse_on_timeout` - how long is the pulse output on after threshold reached
* `pulse_min_interval` - how often at maximum can a device send a packet on pulse event

The use-case of above implementation is somewhat universal, however tailored at monitoring SD card activity in camera traps and similar devices and turning on these SD cards if they have WiFi capability.

## LoraWAN comissioning/provisioning
The device povisioning is done with a separate sketch that uploads the keys to the EEPROM. Then the device gets the actual firmware. This is implemented such that the same firmware build can be distributed publicly and does not contain any key information.

If the keys are incorrect or not provisioned on the device, then the device fill revert to the fallback provisioning specified in the firwmare. Note this is nto secure and you can modify it per project, so no devices should be ever using this. If you see a device responding as fallback you can try to remotely reset it and that is as much as you can do, otherwise you need to reprogram.

Default fallback:
 * `devAddr` = "26011D63"
 * `nwkSKey` = "9518E9E68D1476BC3386409B76476208"
 * `appSKey` = "7972E2A484F76EF7B579D641D0BFEBD5"

## LoraWAN data
See `decoder.js` which is rather human-readable for description of values being sent.

## LoraWAN commands
There are a number of single-byte commands specified for critical device control mechanisms. WARNING: Commands need to be sent unfonfirmed as they may reset the device, loosing the reply.

Implemented commands to be sent as single byte to port 99:
 * `0xaa` - Sent the current settings
 * `0xab` - Triggers system reset
 * `0xde` - Resets LoraWAN stored settings in EEPROM and forces a re-join
 * `0x11` - Request the device GPS position log - 5 positions per packet, all available positions in sequence
 * `0xf1` - Clear GPS position log - use if necessary, but log simply stores last 100 positions and overwrites itself

GPS commands are a separate option on port 91:
 * `0xcc` - Request the GPS to be active based on the downlink command, required additional parameters
   * `duration` - configures how long in minutes should this mode be active
   * `interval` - configures how often in minutes should the unit report position
   * `example binary packet to port 91`: `0xCC 0x05 0x00 0x01 0x00` - send GPS every 1 minute for 5 minutes

## Tools
There are a few tools available to make using this solution easier, namely:
 * `decoder.js` - TheThingsNetwork V2 payload decoder to jet nicely formatted json data from the binary messages
 * `encoder_settings_json.py` -Python scrip to prepare the json structure for settings to be sent to the device
 * `encoder.js` - TheThingsNetwork V2 payload encoder, accepting the above created json

# Building firwmare
The firmware needs to be built with two key things configured:
 * `board.h` - specfying board pinout and functions
 * `lorawan.h` - selecting ABP or OTAA activation

## Power consumption

### System functions 0x00 - all disabled - sleep only with WDT running
Measured consumption 2uA on average, excludes sending Lora messages.

### System functions 0x01 - GPS periodic enabled
Measured consumption 15uA on average, excludes sending Lora messages

## Node-RED integration

Node-RED framework is used as bridge between The Things Network and InfluxDb-Grafana stack.
It formats TTN messages so they can be saved into Influx database and plotted by Grafana.

### port 11 messages

In this port we get several locations in one message, that we have to display as separate points on the Grafan graph so it makes sense to explain how we do it.

For this port message we have to read 5 different locations, each one with its own timestamp, format it and sent it into Influxdb node. Influxdb enables us to write several points at once, as seen on [node-red documentation website](https://flows.nodered.org/node/node-red-contrib-influxdb) about Influxdb.

> **If msg.payload is an array of arrays, it will be written as a series of points containing fields and tags**.

That means that we need 5 arrays, each will contain two objects. First object will contain value fields that we would like to write (lat and lon in this case), second object will contain dev id as usual. We also need time value in first object because each lat and lon pair has different timestamps.
Injected epoch timestamp needs to be 19 digits in order to work with Grafana.

Below code creates functionality that is described above:

```javascript
//clear up payload, we will get our data from payload_fields
msg.payload = []
    
// Locations actualy contains a string not an js object so we need to parse it first.
var parsed_locations = JSON.parse(msg.payload_fields.locations)
    
//iterate through payload_fields, there should be only 5 packets, but we can make this universal
for (i = 0; i < parsed_locations.length; i++)
{
       
    // Make sure that time is correct length so that grafana accepts it
    var epoch_number = parsed_locations[i].time
    var epoch_length = epoch_number.toString().length
    var power_of_ten = 19 - epoch_length
    var correct_grafana_epoch = epoch_number * Math.pow(10, power_of_ten)

        
    //fill up packet that we will push into our msg.payload object
    var lat_lon_packet = [{
        port:   msg.port,
        lat:    parsed_locations[i].lat,
        lon:    parsed_locations[i].lon,
        time:   correct_grafana_epoch,
        rssi:   max_rssi
    },
    {
       devId: msg.dev_id
    }]
    
    msg.payload.push(lat_lon_packet)
}
```
