#include "rf_testing.h"

//#define debug
//#define serial_debug  Serial

boolean rf_send_flag = false;

rf_settingsPacket_t rf_settings_packet;
// 51 to 222 bytes long max packet
// take at most 256 measurements
static uint8_t message[256];

// Configure scanning parameters
float vcc            = 2.5;         //voltage to calculate the measurement

/**
 * @brief Initialize the rf logic
 * 
 */
void rf_init(void){ 
    #ifdef debug
        serial_debug.print("rf_init - ( ");
        serial_debug.println(" )");
    #endif
    pinMode(VSWR_EN,OUTPUT);
    digitalWrite(VSWR_EN,LOW);
}

/**
 * @brief scan vswr values across the band
 * 
 * @param start 
 * @param stop 
 * @param power 
 * @param samples 
 * @param time 
 * @param output 
 * @return boolean 
 */
boolean scan_vswr(uint32_t start, uint32_t stop, int8_t power, uint16_t samples, uint32_t time, uint8_t *output)
{
    uint32_t freq = start;
    uint32_t step = (stop-start)/samples;

    uint32_t measurement_sum = 0;
    uint16_t measurement_count = 0;

    if(step==0 | start>=stop){
        return false;
    }

    for(uint16_t i;i<samples;i++){
        Serial.print("Scan( "); Serial.print(freq); Serial.print(" Hz ");
        // limit time to maximal value
        if(time>4000){
            time=4000;
        }
        // set-up the continuous mode
        LoRaWAN.setTxContinuousWave(freq,power,time);
        // acquire measurements for the specified duration
        unsigned long start_time = millis();
        //wait for stabilization
        delay(10);
        while((millis()-start_time)<time){
            measurement_sum=+analogRead(VSWR_ADC);
            measurement_count++;
            delay(10);
        }

        double value = measurement_sum/measurement_count*vcc/4096; // calculate average and convert to volts
        double dbm=-30; // covnert to dBm
        // secure against division by zero
        if(value!=0){
            dbm = log10(value)-27;
        }
        Serial.print(dbm); Serial.println(" dBm) ");
        // create an 8 bit datapoint use dBm=result/10-30 on the server side
        uint8_t result = (dbm+30)*10;
        *output=result;
        output++;
        //increment frequency
        freq+=step;
        if(freq>stop){
            break;
        }
    }
    return true;
}

/**
 * @brief Send rf values
 * 
 * @return boolean 
 */
boolean rf_send(void){

    #ifdef debug
    serial_debug.print("rf_send( ");
    serial_debug.println(" )");
    #endif
    delay(3000);
    // guard against overflow
    if(rf_settings_packet.data.samples>sizeof(message)){
        rf_settings_packet.data.samples=sizeof(message);
    }
    scan_vswr(rf_settings_packet.data.freq_start, rf_settings_packet.data.freq_stop, rf_settings_packet.data.power, rf_settings_packet.data.samples, rf_settings_packet.data.time, &message[0]);

    delay(5000); // required
    // truncate if payload is longer
    uint16_t length=rf_settings_packet.data.samples;
    if(length>LoRaWAN.getMaxPayloadSize()){
        length=LoRaWAN.getMaxPayloadSize();
    }

    return lorawan_send(rf_vswr_port, &message[0], length);
}