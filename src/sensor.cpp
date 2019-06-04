#include "sensor.h"

//#define debug
//#define serial_debug  Serial

TimerMillis sensor_timer;
TimerMillis sensor_gps_timer_check;
TimerMillis sensor_gps_timer_timeout;

sensorPacket_t sensor_packet;
boolean sensor_send_flag = false;

boolean gps_periodic = false;
boolean gps_triggered = false;
boolean gps_hot_fix = false;
boolean accelerometer_enabled = false;
boolean light_enabled = false;
boolean temperature_enabled = false;
boolean humidity_enabled = false;
boolean pressure_enabled = false;

GNSSLocation sensor_gps_location;
unsigned long sensor_gps_fix_time;

/**
 * @brief called with a timer to perform sensor readign periodically
 * 
 */
void sensor_timer_callback(void)
{
  #ifdef debug
    serial_debug.println("sensor_timer_callback()");
  #endif
  sensor_send_flag = true;
}

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
}

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

void sensor_gps_init(void){
  // GPS
  // Check if GPS is enabled, then set it up accordingly
  if((gps_periodic==true)|(gps_triggered==true)){
    boolean error=false;
    //initialize gps
    pinMode(GPS_EN,OUTPUT);
    digitalWrite(GPS_EN,HIGH);
    pinMode(GPS_BCK,OUTPUT);
    digitalWrite(GPS_BCK,HIGH);
    delay(300); // wait for ublox to boot
    GNSS.end();
    error=sensor_gps_busy_timeout(3000); //if re-initialized this is required
    GNSS.begin(Serial1, GNSS.MODE_UBLOX, GNSS.RATE_1HZ);
    error=sensor_gps_busy_timeout(3000);
    GNSS.setConstellation(GNSS.CONSTELLATION_GPS_AND_GLONASS);
    error=sensor_gps_busy_timeout(3000);

    //set error bits if GPS is not present and self-disable
    if(error){
      // GPS periodic error
      bitSet(status_packet.data.system_functions_errors,0);
      // GPS triggered error
      bitSet(status_packet.data.system_functions_errors,1);
      // Self-disable
      gps_periodic=false;
      gps_triggered=false;
      // Disable GPS power
      digitalWrite(GPS_EN,LOW);
      digitalWrite(GPS_BCK,LOW);
      #ifdef debug
        serial_debug.print("gps init failed");
        serial_debug.println("");
      #endif
      return;
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

    // Accoridng to UBLOX the ephemeris data is valid for maximally 4 hours, if fix interval is greater then that, backup may not be efficient and disablign it automatically in this case.
    // See https://portal.u-blox.com/s/question/0D52p00008HKCT7CAP/time-to-fix-first
    if((gps_hot_fix==true)&(settings_packet.data.sensor_interval<240)){
      digitalWrite(GPS_BCK,HIGH);
    }
    else{
      digitalWrite(GPS_BCK,LOW);
    }
    
    // Processor will be in sleep while waiting for fix, do not wake up from GPS librayr
    GNSS.disableWakeup();
    // This function will be called upon receiving a new GPS location
    GNSS.onLocation(sensor_gps_acquiring_callback);
    // Disable wakeup, not required
    GNSS.disableWakeup();
    // GPS to sleep
    GNSS.suspend();
    // Disable GPS power
    digitalWrite(GPS_EN,LOW);
    digitalWrite(GPS_BCK,LOW);

    #ifdef debug
      serial_debug.print("gps initialized");
      serial_debug.println("");
    #endif
  }
  else{
    // Disable GPS power
    // disable power
    digitalWrite(GPS_EN,LOW);
    // disable backup
    digitalWrite(GPS_BCK,LOW);
  }
}

void sensor_gps_start(void){
  // Start acquiring position
  // Power up GPS
  digitalWrite(GPS_EN,HIGH);
  delay(10);
  GNSS.resume();
  #ifdef debug
    serial_debug.print("gps( started");
    serial_debug.println(" )");
  #endif
}

void sensor_gps_acquiring_callback(void){
  // Check if new location information is available
  if(GNSS.location(sensor_gps_location)){
    float ehpe = sensor_gps_location.ehpe();
    if((sensor_gps_location.fixType() == GNSSLocation::TYPE_3D) && (ehpe <= settings_packet.data.gps_minimal_ehpe) && sensor_gps_location.fullyResolved()){
      // fix acquired
      // wake up the system
      STM32L0.wakeup();
      #ifdef debug
        serial_debug.print("gps ( fix");
        serial_debug.println(" )");
      #endif
    }
  }
}

void sensor_gps_stop(void){
  GNSS.suspend();
  #ifdef debug
    serial_debug.print("gps( stopped");
    serial_debug.println(" )");
  #endif
  // Power off GPS
  digitalWrite(GPS_EN,LOW);
}

/**
 * @brief initialize sensors upon boot or new settings
 * 
 * @details Make sure each sensors is firstlu properly reset and put into sleep and then if enabled in settings, initialize it
 * 
 */
void sensor_init(void){
  sensor_timer.stop();
  sensor_timer.start(sensor_timer_callback, 0, settings_packet.data.sensor_interval*60*1000);

  #ifdef debug
    serial_debug.print("sensor_init - sensor_timer_callback( ");
    serial_debug.print("interval: ");
    serial_debug.print(settings_packet.data.sensor_interval);
    serial_debug.println(" )");
  #endif

  sensor_system_functions_load();

  sensor_gps_init();


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

void sensor_read(void){
  // check if GPS is enabled and then proceed
  if((gps_periodic==true)|(gps_triggered==true)){
    sensor_gps_fix_time=millis();
    // Prepare GPS
    sensor_gps_start();
    // TODO: More detailed handling is required to allow for euphemeris download and ocassional cold fix when scheduled hot fix
    // Go to sleep for hot or cold fix timeout, if fix is acquired or timeout, the sleep will be broken and process resumed
    STM32L0.stop(((gps_hot_fix==true)?settings_packet.data.gps_hot_fix_timeout:settings_packet.data.gps_cold_fix_timeout)*1000);
    // Stop GPS
    sensor_gps_stop();
    // Calculate fix duration
    float time_to_fix = (millis()-sensor_gps_fix_time); //in seconds

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
      serial_debug.print(" lat: "); serial_debug.print(latitude,7);
      serial_debug.print(" lon "); serial_debug.print(longitude,7);
      serial_debug.print(" alt: "); serial_debug.print(altitude,3);
      serial_debug.print(" hdop: "); serial_debug.print(hdop,2);     
      serial_debug.print(" epe: "); serial_debug.print(epe,2);
      serial_debug.print(" sat: "); serial_debug.print(satellites,0); 
      serial_debug.print(" ttf: "); serial_debug.print(time_to_fix); 
      serial_debug.println(" )"); 
    #endif  
  }
  else{
    #ifdef debug
      serial_debug.print("sensor_read(");
      serial_debug.print(" GPS not enabled");
      serial_debug.println(" )"); 
    #endif  
  }
}

/**
 * @brief acquire and send sensor data. 
 * 
 * @details Make sure to do this only for features enabled in the settings
 * 
 */
void sensor_send(void){
    //assemble information
    sensor_read();
    lorawan_send(sensor_packet_port, &sensor_packet.bytes[0], sizeof(sensorData_t));
}