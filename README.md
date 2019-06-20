# smartparks-rgino-tracker-firmware
Smartparks GPS tracker hardware repository, hereby we define features of the system and what needs to be built.

This code is being actively developed and is not yet suitable for use.

# Operation

## LoraWAN commands
There are a number of single-byte commands specified for critical device control mechanisms. WARNING: Commands need to be sent unfonfirmed as they may reset the device, loosing the reply.

Implemented commands to be sent as single byte to port 99:
 * 0xaa - Sent the current settings
 * 0xab - Triggers system reset
 * 0xde - Erases EEPROM and triggers system reset

# Testing
Various test to validate correct operation

## Power consumption

### System functions 0x00 - all disabled - sleep only with WDT running
Measured consumption 2uA on average, excludes sending Lora messages.

### System functions 0x01 - GPS periodic enabled
Measured consumption 30uA on average, excludes sending Lora messages, expecting 15uA with GPS backup on

## Function tests
1. Reports GPS position in defined intervals
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
1. GPS delay before shitdown check

# TODO
1. Implement settings for everything required
1. Hot / cold fix configuration
1. Evaluate power
1. Clean-up

