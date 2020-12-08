#include "gps_tracker.h"

#define debug
#define serial_debug  Serial

gpsPacket_t gps_packet;
gpsLogPacket_t gps_log_packet;

boolean gps_send_flag = false; // extern
boolean gps_log_flag = false; // extarn
boolean gps_done = false; // extern

uint8_t gps_log_count=0;
uint8_t gps_log_send_count=0;

boolean gps_begin_happened = false;
uint8_t gps_fail_count = 0;
uint8_t gps_fail_fix_count = 0;
uint8_t gps_response_fail_count = 0;
boolean gps_hot_fix = false;
unsigned long gps_event_last = 0;
unsigned long gps_accelerometer_last = 0;
unsigned long gps_turn_on_last = 0;
unsigned long gps_on_time_total = 0;

unsigned long gps_command_on_stop = 0;
unsigned long gps_command_interval = 0;

unsigned long gps_fix_start_time = 0;
unsigned long gps_timeout = 0;
unsigned long gps_time_to_fix;

TimerMillis gps_timer_timeout;
TimerMillis gps_timer_response_timeout;

extern GNSSLocation gps_location;
extern GNSSSatellites gps_satellites;

extern statusPacket_t status_packet;

AsyncAPDS9306 lux_sensor;

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
  /*#ifdef debug
    serial_debug.print("gps_accelerometer_interrupt(");
    serial_debug.println(")");
  #endif*/
  gps_accelerometer_last=millis();
}

/**
 * @brief Schedule GPS fix based on the provided timings
 * 
 */
void gps_scheduler(void){
  unsigned long interval=0;
  uint8_t motion_flag=0;
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
    //serial_debug.println("gps_fail_retry");
    return;
  }
 
  // do not schedule a GPS event if battery voltage is under the limit
  if(settings_packet.data.gps_charge_min>status_packet.data.battery){
    //serial_debug.println("status_packet.data.battery");
    return;
  }

  accel_data axis;
  axis = status_accelerometer_read();
  float accel_threshold = settings_packet.data.gps_accel_z_threshold - 2000;

  // do not schedule a GPS event if orientation is bad
  if(accel_threshold!=0){
    if(accel_threshold>0){
      if(accel_threshold>axis.z_axis){
        // do not make a GPS fix as orientation is not right
        //serial_debug.println("do not make a GPS fix as orientation is not right");
        return;
      }
    }
    else if(accel_threshold<axis.z_axis){
        // do not make a GPS fix as orientation is not right
        //serial_debug.println("do not make a GPS fix as orientation is not right");
        return;
      }
  }

  // if triggered gps is enabled and accelerometer trigger has ocurred
  if(settings_packet.data.gps_periodic_interval>0){
    interval=settings_packet.data.gps_periodic_interval;
    motion_flag=0;
  }

  // if manual command trigger GPS has been received - overrides above options
  // upon reception configure the duration of this mode being active and use the provided configuration
  if((millis()<gps_command_on_stop) & (gps_command_on_stop!=0)){
    if(gps_command_interval!=0){
      interval=gps_command_interval;
    }
  }

  // if triggered gps is enabled and accelerometer trigger has ocurred - overrides periodic interval
  if(settings_packet.data.gps_triggered_interval>0){
    if((((millis()-gps_accelerometer_last)/1000)<settings_packet.data.gps_triggered_interval) & (gps_accelerometer_last!=0)){
      interval=settings_packet.data.gps_triggered_interval;
      motion_flag=1;
    }
  }

  // linear backoff upon fail, multiplies the interval with the number of GPS fixes
  if(bitRead(settings_packet.data.gps_settings,1)){
    interval=interval*(gps_fail_fix_count+1);
  }

  // determine the fix interval
  if((interval!=0) & (millis()-gps_event_last>=interval*60*1000|gps_event_last==0)){
    gps_event_last=millis();
    gps_send_flag=true;
    gps_packet.data.motion=motion_flag;
  }
}

void gps_command_request(uint16_t interval, uint16_t duration){
  gps_command_interval=interval; // in minutes as all intervals
  gps_command_on_stop=millis()+duration*60000; // in minutes from the current time converted to milliseconds
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
    gps_turn_on_last=millis();
  }
  else{
    digitalWrite(GPS_EN,LOW);
    delay(100);
    pinMode(GPS_EN,INPUT_PULLDOWN);
    // tracks GPS total on time
    if(gps_turn_on_last>0){
      gps_on_time_total+=(millis()-gps_turn_on_last);
    }
    gps_turn_on_last=0;
  }
}

/**
 * @brief Controls GPS pin for backup power
 * 
 * @param enable 
 */
void gps_backup(boolean enable){
#ifdef GPS_BCK
  if(enable){
    pinMode(GPS_BCK,OUTPUT);
    digitalWrite(GPS_BCK,HIGH);
  }
  else{
    digitalWrite(GPS_BCK,LOW);
    pinMode(GPS_BCK,INPUT_PULLDOWN);
  }
#endif
}

/**
 * @brief test the gps is present and working - not testing gps fixes
 * 
 * @return boolean 
 */
boolean gps_test(){
  if(gps_begin()==false){
    return false;
  }
  delay(3000);
  gps_begin_happened==false;
  gps_end();
  gps_fail_fix_count=0;
  gps_power(false);
  gps_backup(false);
  bitClear(status_packet.data.system_functions_errors,2);
  return true;
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
    #ifdef debug
      serial_debug.println("too large gps_fail_retry");
    #endif
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
  boolean error=false;
  //configure ublox mode and the serial port it is connected to
  GNSS.begin(GNSS.MODE_UBLOX, GNSS.RATE_1HZ, Serial2, -1, GPS_EN, -1);//GPS_EN, GPS_BCK);

  gps_begin_happened=true;
  // enable wakeup allows the gps to wake the processor from sleep
  GNSS.enableWakeup();
  // fail if begin fails to respond
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
  GNSS.setConstellation(GNSS.CONSTELLATION_GPS_AND_GLONASS);
  error|=gps_busy_timeout(1000);

  GNSS.setAutonomous(false);
  error|=gps_busy_timeout(1000);

  GNSS.setAntenna(GNSS.ANTENNA_INTERNAL);
  error|=gps_busy_timeout(1000);

  GNSS.setPlatform(GNSS.PLATFORM_PORTABLE);
  error|=gps_busy_timeout(1000);

  if(error){
    gps_end();
    #ifdef debug
      serial_debug.println("fail at config");
    #endif
    return false;
  }

  // GPS periodic and triggered error and fix
  bitClear(status_packet.data.system_functions_errors,0);
  bitClear(status_packet.data.system_functions_errors,1);
  // make sure the fix error is set as it needs to be cleared on successful fix
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
  #ifdef debug
    serial_debug.print("gps_start(begin");
    serial_debug.println(")");
  #endif

  // Step 0: Initialize GPS
  if(gps_begin_happened==false){
    if(gps_begin()==false){
      return false;
    }
  }
  else{
    // Step 1: Enable power
    gps_power(true);
    delay(100);
    // Step 2: Resume GPS
    gps_busy_timeout(1000);
  }

  // Step 3: Start acquiring location
  gps_fix_start_time=millis();
  // determine the timeout from settings
  if(gps_hot_fix){
    gps_timeout = settings_packet.data.gps_hot_fix_timeout*1000;
  }
  else{
    gps_timeout = settings_packet.data.gps_cold_fix_timeout*1000;
  }

  // Schedule automatic stop after timeout
  gps_timer_timeout.start(gps_stop,gps_timeout+1000);
  // Schedule message reading every second from GPS
  gps_timer_response_timeout.start(gps_acquiring_callback,1000,1000);
  gps_response_fail_count = 0;
  // flag to run with the main FSM
  gps_done = false;

  #ifdef debug
    serial_debug.print("gps_start(end");
    serial_debug.println(")");
  #endif
  return true;
}

/**
 * @brief Callback called by timer to periodically get GPS messages
 * 
 */
void gps_acquiring_callback(void){
  if(GNSS.location(gps_location)){
    float ehpe = gps_location.ehpe();
    boolean gps_3D_fix_required = bitRead(settings_packet.data.gps_settings,0);
    boolean gps_fully_resolved_required = bitRead(settings_packet.data.gps_settings,3);

    #ifdef debug
      serial_debug.print("gps( ehpe ");
      serial_debug.print(ehpe);
      serial_debug.print(" sat ");
      serial_debug.print(gps_location.satellites());
      //serial_debug.print(" aopcfg ");
      //serial_debug.print(gps_location.aopCfgStatus());
      //serial_debug.print(" aop ");
      //serial_debug.print(gps_location.aopStatus());
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
      }
    }
  }
  else{
    // provided there is no data from gps 10 times in sequence, trigger a fault
    gps_response_fail_count++;
    if(gps_response_fail_count>10){
      //raise error and stop
      gps_stop();
    }
  }
}

/**
 * @brief Reads the data from GPS and performs shutdown, called by either the timeout function or the acquiring callback upon fix
 * 
 * @param good_fix - to indicate stopping with good fix acquired
 */
void gps_stop(void){
  gps_timer_timeout.stop();
  gps_timer_response_timeout.stop();
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
    // exception to allow for GPS to never be able to fail if cold fixes do not succeed
    else if(255==settings_packet.data.gps_cold_fix_retry){
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
  
  // process sucessful fix data
  GNSS.satellites(gps_satellites);
  uint8_t max_snr = 0;
  for (unsigned int index = 0; index < gps_satellites.count(); index++){
#ifdef debug
    serial_debug.print("gps_sat(");
    serial_debug.print(gps_satellites.svid(index));
    serial_debug.print(",");
    serial_debug.print(gps_satellites.snr(index));
    serial_debug.print(",");
    serial_debug.print(gps_satellites.acquired(index));
    serial_debug.print(",");
    serial_debug.print(gps_satellites.locked(index));
    serial_debug.print(",");
    serial_debug.print(gps_satellites.navigating(index));
    serial_debug.print(")");
#endif
    if(max_snr<gps_satellites.snr(index)){
      max_snr=gps_satellites.snr(index);
    }
  }
  float latitude, longitude, hdop, epe, satellites, altitude = 0;
  if (gps_location.fixType()>= GNSSLocation::TYPE_2D){
    
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
    
    // position logging

    struct tm timeinfo;
    timeinfo.tm_sec = gps_location.seconds();
    timeinfo.tm_min = gps_location.minutes();
    timeinfo.tm_hour = gps_location.hours();
    timeinfo.tm_mday = gps_location.day();
    timeinfo.tm_mon  = gps_location.month() - 1;
    timeinfo.tm_year = gps_location.year() - 1900;
    time_t time = mktime(&timeinfo);

    gps_log_packet.data[gps_log_count].lat1=gps_packet.data.lat1;
    gps_log_packet.data[gps_log_count].lat2=gps_packet.data.lat2;
    gps_log_packet.data[gps_log_count].lat3=gps_packet.data.lat3;
    gps_log_packet.data[gps_log_count].lon1=gps_packet.data.lon1;
    gps_log_packet.data[gps_log_count].lon2=gps_packet.data.lon2;
    gps_log_packet.data[gps_log_count].lon3=gps_packet.data.lon3;
    gps_log_packet.data[gps_log_count].time=(uint32_t)time;

    gps_log_count++;

    // wrap around if log is full
    if(GPS_LOG_SIZE<=gps_log_count){
      gps_log_count=0;
    }

    gps_packet.data.time=(uint32_t)time;

    status_packet.data.lat1 = gps_packet.data.lat1;
    status_packet.data.lat2 = gps_packet.data.lat2;
    status_packet.data.lat3 = gps_packet.data.lat3;
    status_packet.data.lon1 = gps_packet.data.lon1;
    status_packet.data.lon2 = gps_packet.data.lon2;
    status_packet.data.lon3 = gps_packet.data.lon3;
    status_packet.data.gps_resend = 0;
    status_packet.data.gps_time =(uint32_t)time;
  }
  else{
    gps_packet.data.lat1 = 0;
    gps_packet.data.lat2 = 0;
    gps_packet.data.lat3 = 0;
    gps_packet.data.lon1 = 0;
    gps_packet.data.lon2 = 0;
    gps_packet.data.lon3 = 0;
    gps_packet.data.alt = 0;
  }
  gps_packet.data.satellites_hdop = (((uint8_t)satellites)<<4)|(((uint8_t)hdop)&0x0f);
  gps_packet.data.time_to_fix = (uint8_t)(gps_time_to_fix/1000);
  gps_packet.data.epe = (uint8_t)epe;
  gps_packet.data.snr = (uint8_t)max_snr;
  //gps_packet.data.lux = (uint8_t)get_bits(lux_read(),0,1000,8);
  status_packet.data.gps_on_time_total=gps_on_time_total/1000;

  #ifdef debug
    serial_debug.print("gps(");
    serial_debug.print(" "); serial_debug.print(gps_on_time_total/1000);
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
  #endif

  // if there is a fail, then disable gps and run end script
  if(gps_fail_count>0){
    gps_end();
    return;
  }
}

void gps_end(void){
  // Note: https://github.com/GrumpyOldPizza/ArduinoCore-stm32l0/issues/90
  GNSS.end();
  gps_begin_happened=false;
  gps_power(false);
  gps_backup(false);
  // GPS periodic and triggered error and fix
  bitSet(status_packet.data.system_functions_errors,0);
  bitSet(status_packet.data.system_functions_errors,1);
  bitSet(status_packet.data.system_functions_errors,2);
  // Self-disable
  #ifdef debug
    serial_debug.print("gps_end(");
    serial_debug.print(" "); serial_debug.print(gps_on_time_total);
    serial_debug.println(")");
  #endif
}

/**
 * @brief initialize sensors upon boot or new settings
 * 
 * @details Make sure each sensors is firstlu properly reset and put into sleep and then if enabled in settings, initialize it
 * 
 */
void lux_init(void){
  #ifdef debug
    serial_debug.print("lux_init(");
    serial_debug.println(")");
  #endif

  // if this is enabled and previously an i2c device has been initialized, infinite loop happens
  /*pinMode(PIN_WIRE_SCL,INPUT);
  delay(1000);
  if(digitalRead(PIN_WIRE_SCL)==LOW){
    //no I2C pull-up detected
    bitSet(status_packet.data.system_functions_errors,3);
    #ifdef debug
      serial_debug.print("lux_init(");
      serial_debug.println("i2c error)");
    #endif
    return;
  }*/

  const APDS9306_ALS_GAIN_t again = APDS9306_ALS_GAIN_18;
  const APDS9306_ALS_MEAS_RES_t atime = APDS9306_ALS_MEAS_RES_20BIT_400MS;

  //initialize sensor even if not enabled to put it in low poewr
#ifdef LIGHT_EN
  pinMode(LIGHT_EN,OUTPUT);
  digitalWrite(LIGHT_EN,HIGH);
#endif // LIGHT_EN
  delay(1000);
  if (lux_sensor.begin(again, atime)==false) {
    //set lux error
    bitSet(status_packet.data.system_functions_errors,4);
    #ifdef debug
      serial_debug.print("lux_init(");
      serial_debug.println("lux error)");
    #endif
  }
#ifdef LIGHT_EN
  digitalWrite(LIGHT_EN,LOW);
  pinMode(LIGHT_EN,INPUT_PULLDOWN);
#endif // LIGHT_EN
  return;
}

/**
 * @brief read lux value
 * 
 */
float lux_read(void){
  unsigned long startTime;
  unsigned long duration;
#ifdef LIGHT_EN
  digitalWrite(LIGHT_EN,HIGH);
  delay(100);
#endif // LIGHT_EN

  if(bitRead(status_packet.data.system_functions_errors,4)){
    return 0;
  }

  lux_sensor.startLuminosityMeasurement();

  for(uint16_t i=0;i<100;i++){
    if(lux_sensor.isMeasurementReady()){
      break;
    }
    //STM32L0.deepsleep(10);
    delay(10);
  }

  AsyncAPDS9306Data data = lux_sensor.getLuminosityMeasurement();

  float lux = data.calculateLux();
  #ifdef debug
      serial_debug.print("lux_read(");
      serial_debug.print(lux);
      serial_debug.println(")");
  #endif

#ifdef LIGHT_EN
  digitalWrite(LIGHT_EN,LOW);
  pinMode(LIGHT_EN,INPUT_PULLDOWN);
#endif // LIGHT_EN
  return lux;
}

/**
 * @brief send gps data 
 *  
 */
boolean gps_send(void){
  return lorawan_send(gps_packet_port, &gps_packet.bytes[0], sizeof(gpsData_t));
}

/**
 * @brief send gps log 
 *  
 */
boolean gps_log_send(void){
  //send gps log in batches of 5 locations until all data is sent
  // if there is remaining data to be sent, schedule sending again
  uint8_t logs_per_packet=5;

  uint16_t offset=logs_per_packet*gps_log_send_count*sizeof(gpsLog_t);

  #ifdef debug
    serial_debug.print("gps_log_send(");
    serial_debug.print(gps_log_send_count);
    serial_debug.print(" ");
    serial_debug.print(gps_log_send_count/logs_per_packet);
    serial_debug.print(" ");
    serial_debug.print(gps_log_count);
    serial_debug.print(" ");
    serial_debug.print(gps_log_count/logs_per_packet);
    serial_debug.print(" ");
    serial_debug.print(gps_log_flag);
    serial_debug.print(" ");
    serial_debug.print(offset);
    serial_debug.print(" ");
    serial_debug.println(")");
  #endif

  gps_log_send_count++;
  //reset log send count
  if(GPS_LOG_SIZE>gps_log_send_count*5){
    gps_log_flag=true;
  }
  else{
    gps_log_send_count=0;
  }

  return lorawan_send(gps_log_packet_port, &gps_log_packet.bytes[offset], sizeof(gpsLog_t)*logs_per_packet);
}

/**
 * @brief send gps cloear 
 *  
 */
void gps_log_clear(void){
  gps_log_send_count=0;
  gps_log_count=0;
  for (int i = 0; i < sizeof(gpsLog_t)*GPS_LOG_SIZE; i++)
  {
    gps_log_packet.bytes[i]=0;
  }
}

/**
 * @brief I2C write register
 * 
 * @param reg - register
 * @param val - value
 */
/*/void writeReg(uint8_t reg, uint8_t val)
{
    Wire.beginTransmission(LIS2DH12_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}*/
