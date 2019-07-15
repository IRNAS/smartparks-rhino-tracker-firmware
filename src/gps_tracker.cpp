#include "gps_tracker.h"

#define debug
#define serial_debug  Serial

gpsPacket_t gps_packet;

boolean gps_send_flag = false; // extern
boolean gps_done = false; // extern

boolean gps_begin_happened = false;
uint8_t gps_fail_count = 0;
uint8_t gps_fail_fix_count = 0;
uint8_t gps_response_fail_count = 0;
boolean gps_hot_fix = false;
unsigned long gps_event_last = 0;
unsigned long gps_accelerometer_last = 0;

unsigned long gps_fix_start_time = 0;
unsigned long gps_timeout = 0;
unsigned long gps_time_to_fix;

//TimerMillis gps_timer_timeout;
//TimerMillis gps_timer_response_timeout;

GNSSLocation gps_location;

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
 * @brief updates last motion activity detected time upon interrup from acelerometer
 * 
 */
void gps_accelerometer_interrupt(void){
  #ifdef debug
    serial_debug.print("gps_accelerometer_interrupt(");
    serial_debug.println(")");
  #endif
  gps_accelerometer_last=millis();
}

/**
 * @brief Schedule GPS fix based on the provided timings
 * 
 */
void gps_scheduler(void){
  unsigned long interval=0;
  /*#ifdef debug
    serial_debug.print("gps_scheduler(");
    serial_debug.print("fail: ");
    serial_debug.print(gps_fail_count);
    serial_debug.print(" accel last: ");
    serial_debug.print(gps_accelerometer_last);
    serial_debug.println(")");
  #endif*/

  // do not schedule a GPS event if it has failed more then the specified amount of times
  if(gps_fail_count>settings_packet.data.gps_fail_retry){
    return;
  }

  // if triggered gps is enabled and accelerometer trigger has ocurred
  if(settings_packet.data.gps_periodic_interval>0){
    interval=settings_packet.data.gps_periodic_interval;
  }

  // if triggered gps is enabled and accelerometer trigger has ocurred - overrides periodic interval
  if(settings_packet.data.gps_triggered_interval>0){
    if((millis()-gps_accelerometer_last)<settings_packet.data.gps_triggered_interval){
      interval=settings_packet.data.gps_triggered_interval;
    }
  }

  // linear backoff upon fail
  if(bitRead(settings_packet.data.gps_settings,1)){
    interval=interval*(gps_fail_count+1);
  }

  if((interval!=0) & (millis()-gps_event_last>=interval*60*1000|gps_event_last==0)){
    gps_event_last=millis();
    gps_send_flag=true;
  }
}

/**
 * @brief Checks if GPS is busy and times out after the specified time
 * 
 * @param timeout 
 * @return boolean 
 */
boolean gps_busy_timeout(uint16_t timeout){
  for(uint16_t i=0;i<timeout;i++){
    if(!GNSS.busy()){
      return false;
    }
    delay(1);
  }
  return true;
}

/**
 * @brief Controls GPS pin for main power
 * 
 * @param enable 
 */
void gps_power(boolean enable){
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
void gps_backup(boolean enable){
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
 * @brief Initializes GPS - to be called upon boot or if no backup power is available
 * 
 * @return boolean - true if successful
 */
boolean gps_begin(void){
  #ifdef debug
    serial_debug.print("gps_begin(");
    serial_debug.println(")");
  #endif

  // check if more fails have occured then allowed
  if(gps_fail_count>settings_packet.data.gps_fail_retry){
    return false;
  }

  // Step 1: power up the GPS and backup power
  gps_power(true);
  gps_backup(true);
  delay(1000); // wait for boot and power stabilization
  // Step 2: initialize GPS
  // Note: https://github.com/GrumpyOldPizza/ArduinoCore-stm32l0/issues/86
  // Note: https://github.com/GrumpyOldPizza/ArduinoCore-stm32l0/issues/90
  gps_busy_timeout(1000);
  GNSS.begin(Serial1, GNSS.MODE_UBLOX, GNSS.RATE_1HZ);
  gps_begin_happened=true;
  if(gps_busy_timeout(3000)){
    gps_end();
    gps_fail_count++;
    #ifdef debug
      serial_debug.println("fail after begin");
    #endif
    return false;
  }
  // Step 3: configure GPS
  // see default config https://github.com/GrumpyOldPizza/ArduinoCore-stm32l0/blob/18fb1cc81c6bc91b25e3346595f820985f2267e5/libraries/GNSS/src/utility/gnss_core.c#L2904
  boolean error=false;
  error|=gps_busy_timeout(1000);

  GNSS.setConstellation(GNSS.CONSTELLATION_GPS_AND_GLONASS);
  error|=gps_busy_timeout(1000);

  GNSS.disableWakeup();
  error|=gps_busy_timeout(1000);

  GNSS.setAutonomous(true);
  error|=gps_busy_timeout(1000);

  GNSS.setAntenna(GNSS.ANTENNA_INTERNAL);
  error|=gps_busy_timeout(1000);

  GNSS.setPlatform(GNSS.PLATFORM_PORTABLE);
  error|=gps_busy_timeout(1000);

  GNSS.onLocation(gps_acquiring_callback);
  error|=gps_busy_timeout(1000);
  
  if(error){
    gps_end();
    gps_fail_count++;
    #ifdef debug
      serial_debug.println("fail at config");
    #endif
    return false;
  }

  // GPS periodic and triggered error and fix
  bitClear(status_packet.data.system_functions_errors,0);
  bitClear(status_packet.data.system_functions_errors,1);
  bitSet(status_packet.data.system_functions_errors,2);

  #ifdef debug
    serial_debug.print("gps_begin(");
    serial_debug.println("done)");
  #endif

  return true;
}

/**
 * @brief Starts the GPS acquisition and sets up timeouts
 * 
 * @return boolean 
 */
boolean gps_start(void){
  bitSet(status_packet.data.system_functions_errors,2);
  // Step 0: Initialize GPS
  if(gps_begin_happened==false){
    if(gps_begin()==false){
      return false;
    }
  }
  //re-initialize upon fail
  else if(gps_fail_count>0){
    // This does not work currently due to a bug
    if(gps_begin()==false){
      return false;
    }
  }
  // Step 1: Enable power
  gps_power(true);
  delay(100);
  // Step 2: Resume GPS
  gps_busy_timeout(1000);
  if(GNSS.resume()==false){
    #ifdef debug
      serial_debug.print("gps(resume failed ");
      serial_debug.println(")");
    #endif
    gps_fail_count++;
    return false;
  }

  // Step 3: Start acquiring location
  gps_fix_start_time=millis();
  if(gps_hot_fix){
    gps_timeout = settings_packet.data.gps_hot_fix_timeout*1000;
  }
  else{
    gps_timeout = settings_packet.data.gps_cold_fix_timeout*1000;
  }

  // Schedule automatic stop after timeout
  //gps_timer_timeout.start(gps_stop,gps_timeout+1000);
  // Schedule automatic stop after timeout if no messages received
  //gps_timer_response_timeout.start(gps_acquiring_callback,1000);
  gps_response_fail_count = 0;
  gps_done = false;
  return true;
}

/**
 * @brief Callback triggered by the GPS module upon receiving amessage from it
 * 
 */
void gps_acquiring_callback(void){
  // Restart automatic stop after timeout if no messages received
  //gps_timer_response_timeout.start(gps_acquiring_callback,1000);
  if(millis()-gps_fix_start_time>gps_timeout){
    gps_stop();
  }
  else if(GNSS.location(gps_location)){
    float ehpe = gps_location.ehpe();
    boolean gps_3D_fix_required = bitRead(settings_packet.data.gps_settings,0);
    boolean gps_fully_resolved_required = bitRead(settings_packet.data.gps_settings,3);

    #ifdef debug
      serial_debug.print("gps( ehpe ");
      serial_debug.print(ehpe);
      serial_debug.print(" sat ");
      serial_debug.print(gps_location.satellites());
      serial_debug.print(" aopcfg ");
      serial_debug.print(gps_location.aopCfgStatus());
      serial_debug.print(" aop ");
      serial_debug.print(gps_location.aopStatus());
      serial_debug.print(" d ");
      serial_debug.print(gps_location.fixType());
      serial_debug.print(" res ");
      serial_debug.print(gps_location.fullyResolved());
      serial_debug.println(" )");
    #endif

    gps_time_to_fix = (millis()-gps_fix_start_time); 
    if((gps_location.fixType()>= GNSSLocation::TYPE_2D)&(gps_time_to_fix>=(settings_packet.data.gps_min_fix_time*1000))){
      // clear GPS fix error
      bitClear(status_packet.data.system_functions_errors,2);
      // wait until fix conditions are met
      if(
        (ehpe <= settings_packet.data.gps_min_ehpe)&
        (gps_location.fullyResolved()|gps_fully_resolved_required==false)&
        ((gps_location.fixType() == GNSSLocation::TYPE_3D)|gps_3D_fix_required==false)&
        (gps_time_to_fix>settings_packet.data.gps_min_fix_time)){
         
        /*#ifdef debug
          serial_debug.print("gps(fix");
          serial_debug.print(" ttf ");
          serial_debug.print(gps_time_to_fix);
          serial_debug.print(" min ");
          serial_debug.print(settings_packet.data.gps_min_fix_time);
          serial_debug.println(")");
        #endif */
        if(bitRead(settings_packet.data.gps_settings,2)){
          // enable hot-fix immediately
          gps_hot_fix=true;
        }
        gps_stop();
        //gps_timer_timeout.start(gps_stop,1);
      }
    }
  }
  else{
    gps_response_fail_count++;
    if(gps_response_fail_count>10){
      //raise error and stop
      gps_fail_count++;
      gps_stop();
      //gps_timer_timeout.start(gps_stop,1);
    }
  }
}

/**
 * @brief Reads the data from GPS and performs shutdown, called by either the timeout function or the acquiring callback upon fix
 * 
 * @param good_fix - to indicate stopping with good fix acquired
 */
void gps_stop(void){
  //gps_timer_timeout.stop();
  //gps_timer_response_timeout.stop();
  gps_time_to_fix = (millis()-gps_fix_start_time);
  // Power off GPS
  gps_power(false);
  if(!bitRead(settings_packet.data.gps_settings,2)){
    // Disable GPS if hot-fix mechanism is not used
    gps_hot_fix=false;
  }

  // if GPS fix error
  if(bitRead(status_packet.data.system_functions_errors,2)){
    gps_fail_fix_count++;
    if((gps_hot_fix==true)&(gps_fail_fix_count>=settings_packet.data.gps_hot_fix_retry)){
      //disable hot-fix and rest fail counter
      gps_hot_fix=false;
      gps_fail_fix_count=0;
    }
    else if(gps_fail_fix_count>=settings_packet.data.gps_cold_fix_retry){
      gps_fail_count++;
    }
  }
  else{
    gps_fail_fix_count=0;
    gps_fail_count=0;
  }
  gps_done = true;

  if(gps_fail_count>0){
    gps_hot_fix=false;
    gps_end();
    return; // continuing fails otherwise by reading invalid data locaitons
  }

  float latitude, longitude, hdop, epe, satellites, altitude = 0;
  latitude = gps_location.latitude();
  longitude = gps_location.longitude();
  altitude = gps_location.altitude();
  hdop = gps_location.hdop();
  epe = gps_location.ehpe();
  satellites = gps_location.satellites();

  // 3 of 4 bytes of the variable are populated with data
  uint32_t lat_packed = (uint32_t)(((latitude + 90) / 180.0) * 16777215);
  uint32_t lon_packed = (uint32_t)(((longitude + 180) / 360.0) * 16777215);
  gps_packet.data.lat1 = lat_packed >> 16;
  gps_packet.data.lat2 = lat_packed >> 8;
  gps_packet.data.lat3 = lat_packed;
  gps_packet.data.lon1 = lon_packed >> 16;
  gps_packet.data.lon2 = lon_packed >> 8;
  gps_packet.data.lon3 = lon_packed;
  gps_packet.data.alt = (uint16_t)altitude;
  gps_packet.data.satellites_hdop = (((uint8_t)satellites)<<4)|(((uint8_t)hdop)&0x0f);
  gps_packet.data.time_to_fix = (uint8_t)(gps_time_to_fix/1000);
  gps_packet.data.epe = (uint16_t)epe;
    
  #ifdef debug
    serial_debug.print("gps(");
    serial_debug.print(" "); serial_debug.print(latitude,7);
    serial_debug.print(" "); serial_debug.print(longitude,7);
    serial_debug.print(" "); serial_debug.print(altitude,3);
    serial_debug.print(" h: "); serial_debug.print(hdop,2);     
    serial_debug.print(" e: "); serial_debug.print(epe,2);
    serial_debug.print(" s: "); serial_debug.print(satellites,0); 
    serial_debug.print(" t: "); serial_debug.print(gps_time_to_fix); 
    serial_debug.print(" f: "); serial_debug.print(gps_fail_fix_count); 
    serial_debug.print(" d: "); serial_debug.print(gps_location.fixType());
    serial_debug.print(" h: "); serial_debug.print(gps_hot_fix);
    serial_debug.print(" c: "); serial_debug.print(gps_fail_count);
    serial_debug.println(")");
    serial_debug.flush();
  #endif
}

void gps_end(void){
  // Note: https://github.com/GrumpyOldPizza/ArduinoCore-stm32l0/issues/90
  //GNSS.end();
  gps_begin_happened==false;
  gps_power(false);
  gps_backup(false);
  // GPS periodic and triggered error and fix
  bitSet(status_packet.data.system_functions_errors,0);
  bitSet(status_packet.data.system_functions_errors,1);
  bitSet(status_packet.data.system_functions_errors,2);
  // Self-disable
  #ifdef debug
    serial_debug.print("gps_end(");
    serial_debug.println(")");
  #endif
}


/**
 * @brief initialize sensors upon boot or new settings
 * 
 * @details Make sure each sensors is firstlu properly reset and put into sleep and then if enabled in settings, initialize it
 * 
 */
void accelerometer_init(void){
  #ifdef debug
    serial_debug.print("accelerometer_init(");
    serial_debug.println(")");
  #endif

  if(digitalRead(PIN_WIRE_SCL)==LOW){
    //no I2C pull-up detected
    bitSet(status_packet.data.system_functions_errors,3);
    return;
  }

  //initialize sensor even if not enabled to put it in low poewr
  Wire.begin();
  Wire.beginTransmission(LIS2DH12_ADDR);
  Wire.write(LIS2DW12_WHO_AM_I);
  Wire.endTransmission();
  Wire.requestFrom(LIS2DH12_ADDR, (uint8_t)1);
  uint8_t dummy = Wire.read(); 

  if(dummy!=0x44){
      //set accelerometer error
      bitSet(status_packet.data.system_functions_errors,3);
      return;
  }
  
  // Accelerometer
  if(settings_packet.data.gps_triggered_interval>0){
    //Enable BDU
    //Set full scale +- 2g 
    //Enable activity detection interrupt
    writeReg(LIS2DW12_CTRL2, 0x08);    
    writeReg(LIS2DW12_CTRL6, 0x00);

    writeReg(LIS2DW12_CTRL4_INT1_PAD_CTRL, 0x20);    

    /**The wake-up can be personalized by two key parameters � threshold and duration. 
      * With threshold, the application can set the value which at least one of the axes 
      * has to exceed in order to generate an interrupt. The duration is the number of 
      * consecutive samples for which the measured acceleration exceeds the threshold.
      */    
    uint8_t thr = settings_packet.data.gps_triggered_threshold&0x3f;
    uint8_t dur = settings_packet.data.gps_triggered_duration;
    writeReg(LIS2DW12_WAKE_UP_THS, thr);//6bit value    
    writeReg(LIS2DW12_WAKE_UP_DUR, dur);    

    //Start sensor with ODR 100Hz and in low-power mode 1 
    writeReg(LIS2DW12_CTRL1, 0x10);

    delay(100);

    //Enable interrupt  function
    writeReg(LIS2DW12_CTRL7, 0x20);
  }
  else{
    writeReg(LIS2DW12_CTRL1, 0x00);  
  }
}

/**
 * @brief send gps data 
 *  
 */
boolean gps_send(void){
  return lorawan_send(gps_packet_port, &gps_packet.bytes[0], sizeof(gpsData_t));
}

/**
 * @brief I2C write register
 * 
 * @param reg - register
 * @param val - value
 */
void writeReg(uint8_t reg, uint8_t val)
{
    Wire.beginTransmission(LIS2DH12_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}