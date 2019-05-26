#include "lorawan.h"

//#define debug
//#define serial_debug  Serial

//#define LORAWAN_ABP
#define LORAWAN_OTAA

#ifdef LORAWAN_ABP
// LoraWAN ABP configuration
const char *devAddr = "26011135";
const char *nwkSKey = "5C37BA82481A880863B375ABB418BDF4";
const char *appSKey = "5A2F89ED921E9DF176D57511797C676F";
#endif

#ifdef LORAWAN_OTAA
// LoraWAN ABP configuration
const char *appEui  = "70B3D57ED001CCE3";
const char *appKey  = "003FF34E9F1C8864953D78DCFBBC84F8";
char devEui[32]; // read from the processor
#endif

void lorawan_init(void){
    LoRaWAN.begin(EU868);
    LoRaWAN.addChannel(1, 868100000, 0, 5);
    LoRaWAN.addChannel(2, 868300000, 0, 5);
    LoRaWAN.addChannel(3, 868500000, 0, 5);
    LoRaWAN.addChannel(4, 867100000, 0, 5);
    LoRaWAN.addChannel(5, 867300000, 0, 5);
    LoRaWAN.addChannel(6, 867500000, 0, 5);
    LoRaWAN.addChannel(7, 867900000, 0, 5);
    LoRaWAN.addChannel(8, 867900000, 0, 5);
    LoRaWAN.setDutyCycle(false);
    // LoRaWAN.setAntennaGain(2.0);
    
    LoRaWAN.onJoin(lorawan_joinCallback);
    LoRaWAN.onLinkCheck(lorawan_checkCallback);
    LoRaWAN.onTransmit(lorawan_doneCallback);
    LoRaWAN.onReceive(lorawan_receiveCallback);

    #ifdef LORAWAN_OTAA
    //Get the device ID
    LoRaWAN.getDevEui(devEui, 18);
    LoRaWAN.setSaveSession(true); // this will save the session for reboot, useful if reoot happens with in poor signal conditons
    LoRaWAN.setLinkCheckLimit(48); // number of uplinks link check is sent, 5 for experimenting, 48 otherwise
    LoRaWAN.setLinkCheckDelay(4); // number of uplinks waiting for an answer, 2 for experimenting, 4 otherwise
    LoRaWAN.setLinkCheckThreshold(4); // number of times link check fails to assert link failed, 1 for experimenting, 4 otherwise
    LoRaWAN.joinOTAA(appEui, appKey, devEui);
    #endif

    #ifdef LORAWAN_ABP
    LoRaWAN.joinABP(devAddr, nwkSKey, appSKey);
    #endif

    lorawan_joinCallback(); // call join callback manually to execute all the steps, necessary for ABP or OTAA with saved session
}

int lorawan_send(uint8_t port, const uint8_t *buffer, size_t size){
    int response = 0; 

    if(!LoRaWAN.busy()){

      #ifdef LORAWAN_OTAA
      /*if (!LoRaWAN.linkGateways())
      {
        LoRaWAN.rejoinOTAA();
      }*/
      #endif

      if (LoRaWAN.joined())
      {
        // set datarate, take only the lower 4 bytes, highest bit sets ADR on/off
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
      }
      else{
        #ifdef debug
            serial_debug.println("lorawan_send() lora not joined");
        #endif
      }
    }
    else{
        #ifdef debug
            serial_debug.println("lorawan_send() lora busy");
        #endif
    }
    return response;
}

// Callback on Join failed/success
void lorawan_joinCallback(void)
{
    if (LoRaWAN.joined())
    {
      #ifdef debug
        serial_debug.println("JOINED");
      #endif
      //send all messages out on join
      settings_send_flag = true;
      status_send_flag = true;
      sensor_send_flag = true;
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

// Link check callback, useful for ADR debugging
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

// callback upon receiving data
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
        //handle commands
        if(LoRaWAN.remotePort()==command_get_packet_port()){
            //check if length is correct, single byte expected
            if(size==1){
                // now the settings can be copied into the structure
                command_receive(data[0]);
            }
        }
    }
  }
}

//callback on link lost, handle rejoining schedule here
void lorawan_doneCallback(void)
{
  if (!LoRaWAN.linkGateways())
  {
    #ifdef debug
      serial_debug.println("DISCONNECTED");
    #endif
  }
}