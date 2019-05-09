# smartparks-gps-tracker-hardware
Smartparks GPS tracker hardware repository

### Features:
* GPS positioning
  * status LED next to GPS antenna
* LoRaWAN connectivity
  * internal or external (uFL) antenna 
* primery cell battery
* [optionl] ADC and I2C expansion

### Specification:
* Device Operation Mode:
 1. Acteive
   * send x time faster then normal
 2. Normal
   * send periodicly or on event
 3. Slow
   * send x time slower then normal

### GPS:
 * gps_mode - these values are logicly OR to form the final mode selection
   * 0 = disabled
   * 1 = shedueld
   * 2 = trigger
 * gps_shedueld_interval
   * valuse represent minutes between fixes
   * note: when gps has fix packet is send imediatly
 * gps_cold_fix_timeout
   * value represents seconds, default value 240s, range 0 - 600
 * gps_hot_fix_timeout
   * value represents seconds, default 60, renge 0 - 600
 * gps_minimal_hdop - minimal axepted precision
   * set value for hdop, renge 0 - 30
   * note: if timeout occures hdop is dismissed
   
### Setings:
 * status_send_interval
   * valuse represent minutes between status, range 0 - 1day
   * note: minimum one message paer day, watchdog sayfy machanisam
 * note: ABP keys are factory writtne into flash
 
### LoRaWAN packet structure
 * PORT1: GPS location: [lat] [long] [alt] [hdop] [time_to_fix]
 * PORT2: system status: [battery_level] [temperature]...
 * PORT0: setings: [configuration variables for device]
   * when received saved in flash, upgrade persistant // figure out what happends with setings
   

 

* hardware stack
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

