# smartparks-gps-tracker-hardware
Smartparks GPS tracker hardware repository, hereby we define features of the system and what needs to be built.

### Features:
* GPS positioning
  * acquire a GPS signal
  * status LED next to GPS antenna for deplyment/testing purposes
* LoRaWAN connectivity
  * internal or external (uFL) antenna 
  * 20dBm TX power
  * 868/915MHz
* primary cell battery power
* [optional] ADC and I2C expansion for measurign externall battery voltage of a 3rd party system

### Specification:
* Device Operation Mode:
 * Active
   * send X time faster than normal
 * Normal
   * send periodically or on event
 * Slow
   * send X time slower than normal

### GPS:
 * gps_mode - these values are logically OR to form the final mode selection
   * 0 = disabled
   * 1 = scheduled
   * 2 = trigger
 * gps_scheduled_interval
   * value represent minutes between fixes
   * note: when GPS has fix packet is send immediately
 * gps_cold_fix_timeout
   * value represents seconds, default value 240s, range 0 - 600
 * gps_hot_fix_timeout
   * value represents seconds, default 60, range 0 - 600
 * gps_minimal_hdop - minimal accepted precision
   * set value for hdop, range 0 - 30
   * note: if a timeout occurs hdop is dismissed
   
### Settings:
 * status_send_interval
   * values represent minutes between status, range 0 - 24*60 (1day) in seconds
   * note: minimum one message per day, a watchdog safety mechanism
 * note: ABP keys are factory written into the flash for each device. OTAA can be used with save session method if so defined - may be required for FOTA.
 * mode_active_trigger_threshold
   * motion activity thresholds
 * mode_active_duration
   * expressed either in seconds or numebr of positions - TBD
 * mode_slow_voltage_threshold
   * determined in mV when the tracker should enter this mode, safe default 3000mV
 
### LoRaWAN packet structure:
 * PORT0: setings: [configuration variables for device] - when received saved in flash, upgrade persistent // figure out what happens with settings
   * gps_mode - these values are logically OR to form the final mode selection
   * gps_scheduled_interval
   * gps_cold_fix_timeout
   * gps_hot_fix_timeout
   * gps_minimal_hdop
   * status_send_interval
   * mode_active_trigger_threshold
   * mode_active_duration
   * mode_slow_voltage_threshold
 * PORT1: GPS location: 
   * lat
   * long
   * alt
   * hdop
   * time-to-fix
 * PORT2: system status:
   * mode
   * battery
   * temperature
 * PORT3: commands (to be received)
   * mode - change to defined mode
   * mode_duration - force the change for fixed duration, 0 indicates permanent

### Hardware stack:
 * board1 
   * top side - gps ceramic antena
   * bottom side - LNA matching circuit and GPS (uBlox M8B)
 * board2 - spacer
 * board3
   * power 
   * accelerometer
 * board4 - spacer
 * board5
   * bottom side - MCU + LoRa (Murata) + uFL connector
 * board6
   * top side - uFL and matching circuit
   * bottom side - LoRa antena (antenova) + TAG connect for programming
  
NOTE: Boards are stacked on top of eachother, the connection between them is on the board edge plated half-via with a wire soldered across.

