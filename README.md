# smartparks-rgino-tracker-firmware
Smartparks GPS tracker hardware repository, hereby we define features of the system and what needs to be built.

This code is being actively developed and is not yet suitable for use.

# Requirements:
 * Modified Arduino Core for STM32L0
 * vscode or similar compile tool (Arduino IDE may break things)
 * this repository
 * TheThingsNetwork or other similar LoraWAN network server solution

# Operation
This device is a power-efficeint GPS tracker sending data via LoraWAN. There are three key messages being sent by the device:
 * `settings` - current device settings - only upon boot, change or request
 * `status` - device status message being sent with the specified `status_interval` in minutes - recommended to do this every 24h
 * `sensor` - device sensor data including GPS position with the specified `sensor_interval` in minutes, however there are several other settings that affect this, such as:
   * `mode_slow_voltage_threshold` - where the sensor data may be slowed down or disabled
   * `sensor_interval_active_threshold` - accelerometer based reporting if motion activity is detected
   * `sensor_interval_active` - the interval when the device is active

## LoraWAN data
See `decoder.js` which is rather human-readable for description of values being sent.

## LoraWAN commands
There are a number of single-byte commands specified for critical device control mechanisms. WARNING: Commands need to be sent unfonfirmed as they may reset the device, loosing the reply.

Implemented commands to be sent as single byte to port 99:
 * 0xaa - Sent the current settings
 * 0xab - Triggers system reset
 * 0xde - Erases EEPROM and triggers system reset

## Tools
There are a few tools available to make using this solution easier, namely:
 * `decoder.js` - TheThingsNetwork V2 payload decoder to jet nicely formatted json data from the binary messages
 * `encoder_settings_json.py` -Python scrip to prepare the json structure for settings to be sent to the device
 * `encoder.js` - TheThingsNetwork V2 payload encoder, accepting the above created json

# Testing
Various test to validate correct operation

## Power consumption

### System functions 0x00 - all disabled - sleep only with WDT running
Measured consumption 2uA on average, excludes sending Lora messages.

### System functions 0x01 - GPS periodic enabled
Measured consumption 30uA on average, excludes sending Lora messages, expecting 15uA with GPS backup on

## Function tests
1. Reports GPS position in defined intervals - DONE
1. Reports GPS position on motion activity (define send interval for this)
1. GPS acquisition time is sub 10s at 1h interval
1. Accepts new configuration via network
1. Remote reset command can be sent
1. OTAA parameters can be adjusted or reset
1. Lora signal suitable and working well
1. Survives gracefully the failure of all external componets (GPS, accelerometer, light)
1. Correctly re-joins if link fails
1. LoraWAN join fail backoff/retry period
1. GPS cold-fix check
 * Tested GPS cold-fix timeout and hot-fix timeout - DONE
 * Evaluating when to switch to cold-fix - DONE
 * 
1. GPS delay before shitdown check
1. Does not polute the spectrum when voltage is low

# TODO
1. Implement settings for everything required
1. Implement and test battery voltage measurement
1. Implement hibernation mode
1. Hot / cold fix configuration
1. Evaluate power
1. Implement timeout for a series of unsuccessful hot-fixes and cold-fixes
1. Implement GPS toumeout if no satellites have been found in some time
1. Clean-up

