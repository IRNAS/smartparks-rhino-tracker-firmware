#include "sensor.h"

#define debug
#define serial_debug  Serial

TimerMillis sensor_gps_timeout;

sensorPacket_t sensor_packet;
boolean sensor_send_flag = false;
boolean sensor_gps_initialized = false;
boolean sensor_gps_active = false;
boolean sensor_gps_done = false;
unsigned long event_sensor_last = 0;

// Decoded settings for easy use in the code
boolean gps_periodic = false;
boolean gps_triggered = false;
boolean gps_hot_fix = false;
boolean accelerometer_enabled = false;
boolean light_enabled = false;
boolean temperature_enabled = false;
boolean humidity_enabled = false;
boolean pressure_enabled = false;
boolean gps_settings_d3 = false;
boolean gps_settings_fail_backoff = false;
unsigned long gps_minimum_fix_time = 0;

GNSSLocation sensor_gps_location;
unsigned long sensor_gps_fix_time;
unsigned long sensor_gps_reply_time;
unsigned long sensor_gps_last_fix_time;
unsigned long sensor_gps_fail_fix_count;
boolean sensor_gps_successful_fix = false;
float time_to_fix;

/**
GPS links:
https://portal.u-blox.com/s/question/0D52p00008HKCskCAH/time-to-acquire-gps-position-when-almanac-and-ephemeris-data-be-cleared-
https://portal.u-blox.com/s/question/0D52p00008HKDCuCAP/guarantee-the-next-start-is-a-hot-start

assist offline is enabled by:
https://github.com/GrumpyOldPizza/ArduinoCore-stm32l0/blob/18fb1cc81c6bc91b25e3346595f820985f2267e5/libraries/GNSS/src/utility/gnss_core.c#L2741
see page 52 of above document
check UBX-NAV-AOPSTATUS and hten disable GPS
check if manually disabling backup does loose gps moduel settings, consider using the provided library to do this and test correct operation

*/

/**
 * @brief load system functions form settings to variables
 * 
 */
void sensor_system_functions_load(void){
  // Initialize sensors to low-power mode
  /* system_functions - enable/disable certain features
  *    bit 0 - gps periodic enabled
  *    bit 1 - gps triggered enabled
  *    bit 2 - gps cold fix = 0 / hot fix = 1
  *    bit 3 - accelerometer enabled
  *    bit 4 - light sensor enabled
  *    bit 5 - temperature sensor enabled
  *    bit 6 - humidity sensor enabled
  *    bit 7 - pressure sensor enabled
  */
  uint8_t system_functions = settings_packet.data.system_functions;
  gps_periodic=bitRead(system_functions,0);
  gps_triggered=bitRead(system_functions,1);
  gps_hot_fix=bitRead(system_functions,2);
  accelerometer_enabled=bitRead(system_functions,3);
  light_enabled=bitRead(system_functions,4);
  temperature_enabled=bitRead(system_functions,5);
  humidity_enabled=bitRead(system_functions,6);
  pressure_enabled=bitRead(system_functions,7);

  uint8_t gps_settings = settings_packet.data.gps_settings;
  gps_settings_d3=bitRead(gps_settings,0);
  gps_settings_fail_backoff=bitRead(gps_settings,1);
  gps_minimum_fix_time=(settings_packet.data.gps_settings>>4)&0x0f;
  gps_minimum_fix_time=gps_minimum_fix_time*1000;
}

/**
 * @brief Schedules events based on system settings
 * 
 */
void sensor_scheduler(void){
  unsigned long elapsed = millis()-event_sensor_last;
  uint8_t backoff= gps_settings_fail_backoff ? sensor_gps_fail_fix_count+1 : 1;
  if(elapsed>=(settings_packet.data.sensor_interval*60*1000*backoff)){
    event_sensor_last=millis();
    sensor_send_flag = true;
    #ifdef debug
      serial_debug.print("sensor_scheduler( elapsed ");
      serial_debug.print(elapsed);
      serial_debug.print(" backoff ");
      serial_debug.print(backoff);
      serial_debug.println(" )");
    #endif
  }
}

/**
 * @brief Checks if GPS is busy and times out after the specified time
 * 
 * @param timeout 
 * @return boolean 
 */
boolean sensor_gps_busy_timeout(uint16_t timeout){
  for(uint16_t i=0;i<timeout;i++){
    if(GNSS.busy()){
      delay(1);
    }
    else{
      return false;
    }
  }
  return true;
}

/**
 * @brief Controls GPS pin for main power
 * 
 * @param enable 
 */
void sensor_gps_power(boolean enable){
  if(enable){
    pinMode(GPS_EN,OUTPUT);
    digitalWrite(GPS_EN,HIGH);
  }
  else{
    digitalWrite(GPS_EN,LOW);
    delay(100);
    pinMode(GPS_EN,INPUT_PULLDOWN);
  }
}

/**
 * @brief COntrols GPS pin for backup power
 * 
 * @param enable 
 */
void sensor_gps_backup(boolean enable){
  if(enable){
    pinMode(GPS_BCK,OUTPUT);
    digitalWrite(GPS_BCK,HIGH);
  }
  else{
    digitalWrite(GPS_BCK,LOW);
    pinMode(GPS_BCK,INPUT_PULLDOWN);
  }
}

/**
 * @brief Initializes GPS -  to be called upon boot or if no backup power is available
 * 
 * @return boolean 
 */
boolean sensor_gps_init(void){
  // Check if GPS is enabled, then set it up accordingly
  if((gps_periodic==true)|(gps_triggered==true)){
    #ifdef debug
      serial_debug.print("gps(sensor_gps_init()");
      serial_debug.println(")");
    #endif
    boolean error=true;
    sensor_gps_power(true);
    sensor_gps_backup(true);
    delay(500); // wait for ublox to boot
    for(int i=0;i<3;i++){
      //https://github.com/GrumpyOldPizza/ArduinoCore-stm32l0/issues/86
      // workaround for the above issue implemented in library
      GNSS.begin(Serial1, GNSS.MODE_UBLOX, GNSS.RATE_1HZ);
      error=sensor_gps_busy_timeout(3000); //if re-initialized this is required
      if(!error){
        break;
      }
      else{
        #ifdef debug
          serial_debug.print("gps(begin timeout");
          serial_debug.println(")");
        #endif
        delay(300);
      }
    }
    //set error bits if GPS is not present and self-disable
    if(error){
      sensor_gps_initialized = false;
      GNSS.end();
      sensor_gps_power(false);
      sensor_gps_backup(false);
      // GPS periodic and tiggered error
      bitSet(status_packet.data.system_functions_errors,0);
      bitSet(status_packet.data.system_functions_errors,1);
      // Self-disable
      gps_periodic=false;
      gps_triggered=false;

      #ifdef debug
        serial_debug.print("gps(failed");
        serial_debug.println(")");
      #endif
      return false;
    }
    // TODO initialize accelerometer and setup triggered mode enabled
    if(gps_triggered==true){
      boolean error=true;
      //initialize accelerometer and set up the triggers
      if(error){
        //self-disable if triggered mode is unavailable
        gps_triggered=false;
      }
    }
    
    // see default config https://github.com/GrumpyOldPizza/ArduinoCore-stm32l0/blob/18fb1cc81c6bc91b25e3346595f820985f2267e5/libraries/GNSS/src/utility/gnss_core.c#L2904
    error=sensor_gps_busy_timeout(1000);
    GNSS.setConstellation(GNSS.CONSTELLATION_GPS_AND_GLONASS);
    // Processor will be in sleep while waiting for fix, do not wake up from GPS library
    error=sensor_gps_busy_timeout(1000);
    GNSS.disableWakeup();
    // Enable Assist Now Autonomous mode
    error=sensor_gps_busy_timeout(1000);
    GNSS.setAutonomous(true);
    // Enable Internall antenna
    error=sensor_gps_busy_timeout(1000);
    GNSS.setAntenna(GNSS.ANTENNA_INTERNAL);
    /// Set platform to portable as default
    error=sensor_gps_busy_timeout(1000);
    GNSS.setPlatform(GNSS.PLATFORM_PORTABLE);
    // This function will be called upon receiving a new GPS location
    error=sensor_gps_busy_timeout(1000);
    GNSS.onLocation(sensor_gps_acquiring_callback);
    // initialize lat fix time
    sensor_gps_last_fix_time=0;
    time_to_fix=0;
    sensor_gps_fail_fix_count=0;
    sensor_gps_successful_fix = false;
    sensor_gps_initialized = true;

    #ifdef debug
      serial_debug.print("gps(initialized");
      serial_debug.println(")");
    #endif
    return true;
  }
  else{
    // Disable GPS power
    sensor_gps_power(false);
    sensor_gps_backup(false);
    // GPS periodic error
    bitSet(status_packet.data.system_functions_errors,0);
    bitSet(status_packet.data.system_functions_errors,1);
    #ifdef debug
      serial_debug.print("gps(disabled");
      serial_debug.println(")");
    #endif
    return false;
  }
}

/**
 * @brief Starts the GPS acquisition and sets up timeouts
 * 
 * @return boolean 
 */
boolean sensor_gps_start(void){
  boolean response = false;
  sensor_gps_active = false;
  sensor_gps_done = false;
  sensor_gps_successful_fix = false;
    // check if GPS is enabled and then proceed
  if((gps_periodic==true)|(gps_triggered==true)){
    // make sure GPS is reset, note backup power is still on
    if(sensor_gps_initialized == true){
      sensor_gps_power(true);
      delay(100);
      sensor_gps_busy_timeout(1000);
      response=GNSS.resume();
    }
    else{
      response=sensor_gps_init();
    }

    if(response==false){
      #ifdef debug
        serial_debug.print("gps(resume failed ");
        serial_debug.println(")");
      #endif
      return false;
    }

    sensor_gps_fix_time=millis();
    // Start acquiring position

    // Go to sleep for hot or cold fix timeout, if fix is acquired or timeout, the sleep will be broken and process resumed
    long sensor_timeout = 0;
    // Hot fix timeout only if enabled hot fix, last fix time is not 0 and no more then 4 since last fix
    unsigned long elapsed = millis()-sensor_gps_last_fix_time;
    if(gps_hot_fix==true & sensor_gps_last_fix_time!=0 & elapsed<=14400000){
        sensor_timeout = settings_packet.data.gps_hot_fix_timeout*1000;
      }
    else{
        sensor_timeout = settings_packet.data.gps_cold_fix_timeout*1000;
    }
    sensor_gps_timeout.start(sensor_gps_stop,sensor_timeout);

    #ifdef debug
      serial_debug.print("gps(started, timeout: ");
      serial_debug.print(sensor_timeout);
      serial_debug.print(" , ehpe ");
      serial_debug.print(settings_packet.data.gps_minimal_ehpe);
      serial_debug.print(" , sensor_gps_last_fix_time ");
      serial_debug.print(sensor_gps_last_fix_time);
      serial_debug.print(" , gps_hot_fix ");
      serial_debug.print(gps_hot_fix);
      serial_debug.println(")");
    #endif
    return true;
  }
  return false;
}

/**
 * @brief Callback triggered by the GPS module upon receiving amessage from it
 * 
 */
void sensor_gps_acquiring_callback(void){
  //flag that gps is active
  sensor_gps_active = true;
  // Check if new location information is available
  if(GNSS.location(sensor_gps_location)){
    float ehpe = sensor_gps_location.ehpe();
    /*#ifdef debug
      serial_debug.print("gps( ehpe ");
      serial_debug.print(ehpe);
      serial_debug.print(" sat ");
      serial_debug.print(sensor_gps_location.satellites());
      serial_debug.print(" aopcfg ");
      serial_debug.print(sensor_gps_location.aopCfgStatus());
      serial_debug.print(" aop ");
      serial_debug.print(sensor_gps_location.aopStatus());
      serial_debug.println(" )");
    #endif*/
    if(((sensor_gps_location.fixType() == GNSSLocation::TYPE_2D)&!gps_settings_d3)|(sensor_gps_location.fixType() == GNSSLocation::TYPE_3D)){
      if((ehpe <= settings_packet.data.gps_minimal_ehpe) && sensor_gps_location.fullyResolved()){
        // fix acquired
        // Stop GPS
        // Calculate fix duration
        time_to_fix = (millis()-sensor_gps_fix_time);
        #ifdef debug
          serial_debug.print("gps(fix");
          serial_debug.print(" ttf ");
          serial_debug.print(time_to_fix);
          serial_debug.print(" min ");
          serial_debug.print(gps_minimum_fix_time);
          serial_debug.println(")");
        #endif
        if(time_to_fix>gps_minimum_fix_time){
          sensor_gps_stop(true);
          sensor_gps_last_fix_time = millis(); // store time of last successful fix
        }
      }
    }
  }
}

/**
 * @brief Reads the data from GPS and performs shutdown, called by either the timeout function or the acquiring callback upon fix
 * 
 */

void sensor_gps_stop(){
  sensor_gps_stop(false);
}

/**
 * @brief Reads the data from GPS and performs shutdown, called by either the timeout function or the acquiring callback upon fix
 * 
 * @param good_fix - to indicate stopping with good fix acquired
 */
void sensor_gps_stop(boolean good_fix){
  sensor_gps_timeout.stop();
  sensor_gps_busy_timeout(100);
  time_to_fix = (millis()-sensor_gps_fix_time);

  // evaluate errors if the fix has not been acquired but a timeout occurred
  if(good_fix==false){
    //increment by one if timeout occurred
    sensor_gps_fail_fix_count++;
    //if no satellites have been heard, then increment again
    if(sensor_gps_location.satellites()==0){
      sensor_gps_fail_fix_count++;
    }
    // fail-back to cold-fix after 10 failed hot-fixes
    if(sensor_gps_fail_fix_count>10){
      sensor_gps_last_fix_time=0; // forcing cold-fix
    }
  }
  else{
    sensor_gps_fail_fix_count=0;
  }
  // Accoridng to UBLOX the ephemeris data is valid for maximally 4 hours, if fix interval is greater then that, backup may not be efficient and disablign it automatically in this case.
  // See https://portal.u-blox.com/s/question/0D52p00008HKCT7CAP/time-to-fix-first
  if(gps_hot_fix==true){
    GNSS.suspend();
    sensor_gps_backup(true);
  }
  else{
    sensor_gps_initialized = false;
    GNSS.end();
    sensor_gps_backup(false);
  }
  // Power off GPS
  sensor_gps_power(false);
  
  // flag gps is inactive and done
  sensor_gps_active = false;
  sensor_gps_done = true;

  /*#ifdef debug
    serial_debug.print("gps(stopped");
    serial_debug.println(")");
  #endif*/

  float latitude, longitude, hdop, epe, satellites, altitude = 0;
  latitude = sensor_gps_location.latitude();
  longitude = sensor_gps_location.longitude();
  altitude = sensor_gps_location.altitude();
  hdop = sensor_gps_location.hdop();
  epe = sensor_gps_location.ehpe();
  satellites = sensor_gps_location.satellites();

  // 3 of 4 bytes of the variable are populated with data
  uint32_t lat_packed = (uint32_t)(((latitude + 90) / 180.0) * 16777215);
  uint32_t lon_packed = (uint32_t)(((longitude + 180) / 360.0) * 16777215);
  sensor_packet.data.lat1 = lat_packed >> 16;
  sensor_packet.data.lat2 = lat_packed >> 8;
  sensor_packet.data.lat3 = lat_packed;
  sensor_packet.data.lon1 = lon_packed >> 16;
  sensor_packet.data.lon2 = lon_packed >> 8;
  sensor_packet.data.lon3 = lon_packed;
  sensor_packet.data.alt = (uint16_t)altitude;
  sensor_packet.data.satellites_hdop = (((uint8_t)satellites)<<4)|(((uint8_t)hdop)&0x0f);
  sensor_packet.data.time_to_fix = (uint8_t)(time_to_fix/1000);
  sensor_packet.data.epe = (uint16_t)epe;
    
  #ifdef debug
    serial_debug.print("gps(");
    serial_debug.print("lat: "); serial_debug.print(latitude,7);
    serial_debug.print(" lon "); serial_debug.print(longitude,7);
    serial_debug.print(" alt: "); serial_debug.print(altitude,3);
    serial_debug.print(" hdop: "); serial_debug.print(hdop,2);     
    serial_debug.print(" epe: "); serial_debug.print(epe,2);
    serial_debug.print(" sat: "); serial_debug.print(satellites,0); 
    serial_debug.print(" ttf: "); serial_debug.print(time_to_fix); 
    serial_debug.print(" ffc: "); serial_debug.print(sensor_gps_fail_fix_count); 
    serial_debug.println(")"); 
  #endif  
}

/**
 * @brief initialize sensors upon boot or new settings
 * 
 * @details Make sure each sensors is firstlu properly reset and put into sleep and then if enabled in settings, initialize it
 * 
 */
void sensor_init(void){
  #ifdef debug
    serial_debug.print("sensor_init(");
    serial_debug.println(")");
  #endif

  sensor_system_functions_load();


  Wire.begin();
  Wire.beginTransmission(LIS2DH12_ADDR);
  Wire.write(LIS2DW12_WHO_AM_I);
  Wire.endTransmission();
  Wire.requestFrom(LIS2DH12_ADDR, (uint8_t)1);
  uint8_t dummy = Wire.read(); 

  if(dummy!=0x44){
      #ifdef debug
        serial_debug.print("LIS not found: 0x");
        serial_debug.println(dummy,HEX);
      #endif
  }

  #ifdef debug
      serial_debug.print("LIS present: 0x");
      serial_debug.println(dummy,HEX);
  #endif

  Wire.beginTransmission(LIS2DH12_ADDR);
  Wire.write(LIS2DW12_CTRL1);
  Wire.write(0x00);
  Wire.endTransmission();
  
  // Accelerometer
  if(accelerometer_enabled==true){
    // check if present
    // configure 
  }
  else{
    // put into low power
  }

  // Light sensor
  if(light_enabled==true){
    // check if present
    // configure 
  }
  else{
    // put into low power
  }   
}

/**
 * @brief Function to read sensor data - all but GPS
 * 
 * @return boolean 
 */
boolean sensor_read(void){
  return true;
}

/**
 * @brief acquire and send sensor data. 
 * 
 * @details Make sure to do this only for features enabled in the settings
 * 
 */
boolean sensor_send(void){
  return lorawan_send(sensor_packet_port, &sensor_packet.bytes[0], sizeof(sensorData_t));
}