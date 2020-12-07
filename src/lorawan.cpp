#include "lorawan.h"
#include "avr/eeprom.h"
#include "wiring_private.h"

//#define debug
//#define serial_debug  Serial

// All keys are provisionted to memory with special firmware.

// This is FALLBACK only:
// LoraWAN ABP configuration - Keys are stored in program memory - this is fallback
const char *devAddr = "26013A07";
const char *nwkSKey = "A05EBD16950988D4884F525DC54B11FA";
const char *appSKey = "9B01ABF4011F3A25B8FD8DD95EFC7E17";

boolean lorawan_send_successful = false; // flags sending has been successful to the FSM

/**
 * @brief Initialize LoraWAN communication, returns fales if fails
 * 
 * @return boolean 
 */
boolean lorawan_init(void){
  if(LoRaWAN.begin(EU868)==0){
    return false;
  }
  // Commented out, we only need one channel
  LoRaWAN.addChannel(1, 868100000, 0, 5);
  LoRaWAN.addChannel(2, 868300000, 0, 5);
  LoRaWAN.addChannel(3, 868500000, 0, 5);
  LoRaWAN.addChannel(4, 867100000, 0, 5);
  LoRaWAN.addChannel(5, 867300000, 0, 5);
  LoRaWAN.addChannel(6, 867500000, 0, 5);
  LoRaWAN.addChannel(7, 867900000, 0, 5);
  LoRaWAN.addChannel(8, 867900000, 0, 5);
  LoRaWAN.setDutyCycle(false);
  LoRaWAN.setMaxEIRP(22);
  LoRaWAN.setAntennaGain(-5.0);
  LoRaWAN.setTxPower(20);
  
  LoRaWAN.onJoin(lorawan_joinCallback);
  LoRaWAN.onLinkCheck(lorawan_checkCallback);
  LoRaWAN.onTransmit(lorawan_doneCallback);
  LoRaWAN.onReceive(lorawan_receiveCallback);

  LoRaWAN.setSaveSession(true); // this will save the session for reboot, useful if reboot happens with in poor signal conditons
  int join_success=0;

  //#ifdef LORAWAN_OTAA
  //Get the device ID
  //LoRaWAN.getDevEui(devEui, 18);
  //LoRaWAN.setLinkCheckLimit(48); // number of uplinks link check is sent, 5 for experimenting, 48 otherwise
  //LoRaWAN.setLinkCheckDelay(4); // number of uplinks waiting for an answer, 2 for experimenting, 4 otherwise
  //LoRaWAN.setLinkCheckThreshold(4); // number of times link check fails to assert link failed, 1 for experimenting, 4 otherwise
  //LoRaWAN.joinOTAA(appEui, appKey, devEui);

  //check stored keys to figure out what mode the device is configured in
  
  /*typedef struct {
    uint32_t Header;
    uint8_t  AppEui[8];
    uint8_t  AppKey[16];
    uint8_t  DevEui[8];
    uint32_t DevAddr;
    uint8_t  NwkSKey[16];
    uint8_t  AppSKey[16];
    uint32_t Crc32;
} LoRaWANCommissioning;*/

  uint8_t key = 0;
  for(uint32_t i=0;i<8;i++){
    key|=EEPROM.read(EEPROM_OFFSET_COMMISSIONING+4+i);
  }
  // if OTAA AppEUi is defined, try to join OTAA
  if(0x01!=key){
    join_success=LoRaWAN.joinOTAA();
    #ifdef debug
      serial_debug.print("joinOTAA:");
      serial_debug.println(join_success);
    #endif
  }
  else{ // try ABP
    key = 0;
    for(int i=0;i<4;i++){
      key|=EEPROM.read(EEPROM_OFFSET_COMMISSIONING+32+i);
    }
    // if ABP DevAddr is defined, try to join ABP
    if(0!=key){
      join_success=LoRaWAN.joinABP();
      #ifdef debug
        serial_debug.print("joinABP:");
        serial_debug.println(join_success);
      #endif
    }
  }

  // fallback to default ABP keys
  if(0==join_success){
    return LoRaWAN.joinABP(devAddr, nwkSKey, appSKey);
  }

  return true;
}

/**
 * @brief Sends the provided data buffer on the given port, returns false if failed
 * 
 * @param port 
 * @param buffer 
 * @param size 
 * @return boolean 
 */
boolean lorawan_send(uint8_t port, const uint8_t *buffer, size_t size){
  #ifdef debug
    serial_debug.println("lorawan_send() init");
  #endif

  // DTC tuning logic

  // Tuning capacitor
  uint8_t eeprom_dtc_1=EEPROM.read(EEPROM_DATA_START_SETTINGS+1);
  if(eeprom_dtc_1!=0){
    #ifdef debug
      serial_debug.print("DTC from eeprom: ");
      serial_debug.println(eeprom_dtc_1);
    #endif
  }
  else
  {
    eeprom_dtc_1=4; // default value assigned
  }

  #ifdef debug
      serial_debug.print("DTC: ");
      serial_debug.println(eeprom_dtc_1);
    #endif
    DTC_Initialize(STM32L0_GPIO_PIN_PB12, eeprom_dtc_1, STM32L0_GPIO_PIN_NONE, 0b0); //TODO

  int response = 0; 
  if (!LoRaWAN.joined()) {
    #ifdef debug
      serial_debug.println("lorawan_send() not joined");
    #endif
    return false;
  }

  if (LoRaWAN.busy()) {
    #ifdef debug
      serial_debug.println("lorawan_send() busy");
    #endif
    return false;
  }

  LoRaWAN.setADR(settings_packet.data.lorawan_datarate_adr>>7);
  LoRaWAN.setDataRate(settings_packet.data.lorawan_datarate_adr&0x0f);
  #ifdef debug
    serial_debug.print("lorawan_send( ");
    serial_debug.print("TimeOnAir: ");
    serial_debug.print(LoRaWAN.getTimeOnAir());
    serial_debug.print(", NextTxTime: ");
    serial_debug.print(LoRaWAN.getNextTxTime());
    serial_debug.print(", MaxPayloadSize: ");
    serial_debug.print(LoRaWAN.getMaxPayloadSize());
    serial_debug.print(", DR: ");
    serial_debug.print(LoRaWAN.getDataRate());
    serial_debug.print(", TxPower: ");
    serial_debug.print(LoRaWAN.getTxPower(), 1);
    serial_debug.print("dbm, UpLinkCounter: ");
    serial_debug.print(LoRaWAN.getUpLinkCounter());
    serial_debug.print(", DownLinkCounter: ");
    serial_debug.print(LoRaWAN.getDownLinkCounter());
    serial_debug.print(", Port: ");
    serial_debug.print(port);
    serial_debug.print(", Size: ");
    serial_debug.print(size);
    serial_debug.println(" )");
  #endif
  // int sendPacket(uint8_t port, const uint8_t *buffer, size_t size, bool confirmed = false);
  response = LoRaWAN.sendPacket(port, buffer, size, false);
  if(response>0){
    #ifdef debug
      serial_debug.println("lorawan_send() sendPacket");
    #endif
    lorawan_send_successful = false;
    return true;
  }
  return false;
}

/**
 * @brief Callback ocurring when join has been successful
 * 
 */
void lorawan_joinCallback(void)
{
    if (LoRaWAN.joined())
    {
      #ifdef debug
        serial_debug.println("JOINED");
      #endif
      LoRaWAN.setRX2Channel(869525000, 3); // SF12 - 0 for join, then SF 9 - 3, see https://github.com/TheThingsNetwork/ttn/issues/155
    }
    else
    {
      #ifdef debug
        serial_debug.println("REJOIN( )");
      #endif
      LoRaWAN.rejoinOTAA();
    }
}

/**
 * @brief returns the boolean status of the LoraWAN join
 * 
 * @return boolean 
 */
boolean lorawan_joined(void)
{
  return LoRaWAN.joined();
}

/**
 * @brief Callback occurs when link check packet has been received
 * 
 */
void lorawan_checkCallback(void)
{
  #ifdef debug
    serial_debug.print("CHECK( ");
    serial_debug.print("RSSI: ");
    serial_debug.print(LoRaWAN.lastRSSI());
    serial_debug.print(", SNR: ");
    serial_debug.print(LoRaWAN.lastSNR());
    serial_debug.print(", Margin: ");
    serial_debug.print(LoRaWAN.linkMargin());
    serial_debug.print(", Gateways: ");
    serial_debug.print(LoRaWAN.linkGateways());
    serial_debug.println(" )");
  #endif
}

/**
 * @brief Callback when the data is received via LoraWAN - procecssing various content
 * 
 */
void lorawan_receiveCallback(void)
{
  #ifdef debug
    serial_debug.print("RECEIVE( ");
    serial_debug.print("RSSI: ");
    serial_debug.print(LoRaWAN.lastRSSI());
    serial_debug.print(", SNR: ");
    serial_debug.print(LoRaWAN.lastSNR());
  #endif
  if (LoRaWAN.parsePacket())
  {
    uint32_t size;
    uint8_t data[256];

    size = LoRaWAN.read(&data[0], sizeof(data));

    if (size)
    {
      data[size] = '\0';
      #ifdef debug
          serial_debug.print(", PORT: ");
          serial_debug.print(LoRaWAN.remotePort());
          serial_debug.print(", SIZE: \"");
          serial_debug.print(size);
          serial_debug.print("\"");
          serial_debug.println(" )");
      #endif

      //handle settings
      if(LoRaWAN.remotePort()==settings_get_packet_port()){
          //check if length is correct
          if(size==sizeof(settingsData_t)){
              // now the settings can be copied into the structure
              memcpy(&settings_packet_downlink.bytes[0],&data, sizeof(settingsData_t));
              settings_from_downlink();
          }
      }
      //handle rf testing
      if(LoRaWAN.remotePort()==rf_vswr_port){
          //check if length is correct
          if(size==sizeof(rf_settingsData_t)){
              // now the settings can be copied into the structure
              memcpy(&rf_settings_packet.bytes[0],&data, sizeof(rf_settingsData_t));
              rf_send_flag=true;
          }
      }
      //handle commands
      if(LoRaWAN.remotePort()==command_get_packet_port()){
          //check if length is correct, single byte expected
          if(size==1){
              // now the settings can be copied into the structure
              command_receive(data[0]);
          }
      }
      // handle GPS commands
      if(LoRaWAN.remotePort()==91){
        if(size==5){
          if(data[0]=0xcc){
            uint16_t interval = data[1]|data[2]<<8;
            uint16_t duration = data[3]|data[4]<<8;
            gps_command_request(interval,duration);
          }
        }
      }
      // handle set DTC value
      if(LoRaWAN.remotePort()==92){
        if(size==1){
          lorawan_set_dtc(data[0]);
        }
      }
      // handle calibrate ADS
      if(LoRaWAN.remotePort()==93){
        if(size==2){
          uint16_t calibrate_value= data[1]|data[0]<<8;
          status_fence_monitor_calibrate(calibrate_value);
        }
      }
    }
  }
}

/**
 * @brief Callback on transmission done - signals successful TX with a flag
 * 
 */
void lorawan_doneCallback(void)
{
  #ifdef debug
    serial_debug.println("DONE()");
  #endif

  if (!LoRaWAN.linkGateways())
  {
    #ifdef debug
      serial_debug.println("DISCONNECTED");
    #endif
  }
  else{
    lorawan_send_successful = true;
  }
}

void lorawan_set_dtc(uint8_t dtc_value){
  // make sure the value is not zero
  if(dtc_value==0){
    return;
  }
  if(dtc_value>32){
    return;
  }
  EEPROM.write(EEPROM_DATA_START_SETTINGS+1,dtc_value);
}
