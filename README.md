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
   * `system_function_erros` - indicate error of different sub-systems
     * bit 0 - gps periodic error - sent when there is an issue comunicating with the GPS
     * bit 1 - gps triggered error
     * bit 2 - gps fix error - set if no fix has been acquired at all - note if 3D fix or target epe is not acquired we still consider this a valid fix
     * bit 3 - accelerometer error
     * bit 4 - light sensor error
     * bit 5 - temperature error
     * bit 6 - humidity sensor error
     * bit 7 - pressure sensor error
 * `gps` - gps location - gps packet is set based on the `settings` and various intervals defined there.

## Reed switch
The magnetic REED switch is put in place to put the device in hibernation for transport purposes or prior to installation. Removing the magnet will initialize the operation in 60s of removing it. Note that putting the device in hibernation performs a system reset, including the provisioned settings. Provide settings via the network.

## GPS acquisition logic
The GPS system is configured by defining the `gps_periodic_interval` and other variables.


## LoraWAN data
See `decoder.js` which is rather human-readable for description of values being sent.

## LoraWAN commands
There are a number of single-byte commands specified for critical device control mechanisms. WARNING: Commands need to be sent unfonfirmed as they may reset the device, loosing the reply.

Implemented commands to be sent as single byte to port 99:
 * 0xaa - Sent the current settings
 * 0xab - Triggers system reset
 * 0xde - Resets LoraWAN stored settings in EEPROM and forces a re-join

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
1. Accepts new configuration via network - DONE
1. Remote reset command can be sent - DONE
1. OTAA parameters can be adjusted or reset - DONE
1. Lora signal suitable and working well
1. Survives gracefully the failure of all external componets (GPS, accelerometer, light) - DONE
1. Correctly re-joins if link fails - NOT REQUIRED
1. LoraWAN join fail backoff/retry period - NOT REQUIRED
1. GPS cold-fix check -DONE
 * Tested GPS cold-fix timeout and hot-fix timeout - DONE
 * Evaluating when to switch to cold-fix - DONE
1. GPS delay before shutdown check - DONE
1. Does not polute the spectrum when voltage is low - DONE

# TODO
1. Implement settings for everything required
1. Implement and test battery voltage measurement
1. Evaluate power
1. Clean-up

