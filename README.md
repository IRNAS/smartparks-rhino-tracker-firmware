# smartparks-gps-tracker-hardware
Smartparks GPS tracker hardware repository

### Features:
* GPS positioning
  * status LED next to GPS antenna
* LoRaWAN connectivity
  * internal or external (uFL) antenna 
* primary cell battery
* [optionl] ADC and I2C expansion

### Specification:
* Device Operation Mode:
 1. Active
   * send x time faster than normal
 2. Normal
   * send periodically or on event
 3. Slow
   * send x time slower than normal

### GPS:
 * gps_mode - these values are logically OR to form the final mode selection
   * 0 = disabled
   * 1 = scheduled
   * 2 = trigger
 * gps_shedueld_interval
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
   * values represent minutes between status, range 0 - 1day
   * note: minimum one message per day, a watchdog safety mechanism
 * note: ABP keys are factory written into the flash
 
### LoRaWAN packet structure:
 * PORT1: GPS location: [lat] [long] [alt] [hdop] [time_to_fix]
 * PORT2: system status: [battery_level] [temperature]...
 * PORT0: setings: [configuration variables for device]
   * when received saved in flash, upgrade persistent // figure out what happens with settings
   

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
  
NOTE: Boards are stacked. //expain this

