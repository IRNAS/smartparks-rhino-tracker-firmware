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
        #ifdef debug
            serial_debug.print("Scan( "); serial_debug.print(freq); serial_debug.print(" Hz ");
        #endif
        // limit time to maximal value
        if(time>4000){
            time=4000;
        }
        // set-up the continuous mode
        for(uint16_t i=0;i<5000;i++){
            if(LoRaWAN.setTxContinuousWave(freq,power,time)==false){
                break;
            }
            delay(100);
        }
        // acquire measurements for the specified duration
        unsigned long start_time = millis();
        //wait for stabilization
        delay(10);
        while((millis()-start_time)<time){
            measurement_sum+=analogRead(VSWR_ADC);
            measurement_count++;
            delay(10);
        }

        double value = measurement_sum/measurement_count*vcc/4096; // calculate average and convert to volts
        double dbm=-31; // covnert to dBm
        // secure against division by zero
        if(value!=0){
            dbm = log10(value)-27;
        }
        #ifdef debug
            serial_debug.print(dbm); serial_debug.println(" dBm) ");
        #endif
        // create an 8 bit datapoint use dBm=result/10-30 on the server side
        uint8_t result = (dbm+31)*10;
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

    //inplementing different types of measurements
    // type=0 set tuning value
    // type=1 rf_autotune
    // type=2 vswr scan frequency
    digitalWrite(VSWR_EN,HIGH);
    delay(3000);
    // guard against overflow
    if(rf_settings_packet.data.samples>sizeof(message)){
        rf_settings_packet.data.samples=sizeof(message);
    }
    scan_vswr(rf_settings_packet.data.freq_start, rf_settings_packet.data.freq_stop, rf_settings_packet.data.power, rf_settings_packet.data.samples, rf_settings_packet.data.time, &message[0]);
    digitalWrite(VSWR_EN,LOW);
    delay(5000); // required
    // truncate if payload is longer
    uint16_t length=rf_settings_packet.data.samples;
    if(length>LoRaWAN.getMaxPayloadSize()){
        length=LoRaWAN.getMaxPayloadSize();
    }

    return lorawan_send(rf_vswr_port, &message[0], length);
}

boolean rf_autotune(void){
    float tune_start  = 0;
    float tune_stop   = 15;
    float tune_step   = 1;
    uint8_t *output = &message[0];
    uint8_t length = 0;
    float vcc = 2.5;
    pinMode(VSWR_EN,OUTPUT);
    digitalWrite(VSWR_EN,HIGH);

  for(float tune=0;tune<tune_stop;tune++){
    delay(1000);
    STM32L0.wdtReset(); // just a hack due to a large delay in this loop
    DTC_Initialize(STM32L0_GPIO_PIN_PB12, tune, STM32L0_GPIO_PIN_NONE, 0b0);
    LoRaWAN.setTxContinuousWave(868000000,5,100);
    unsigned long start_time=millis();
    unsigned long elapsed=millis()-start_time;
    float avg=0;
    float count = 0;
    while(elapsed<110){
        elapsed=millis()-start_time;
        int adc=analogRead(VSWR_ADC);
        if(adc>0){
        avg+=adc;
        count++;
        }
    }
    *output=avg/count/10;
    output++;
    length++;
    Serial.print(tune);
    Serial.print(",");
    Serial.println(avg/count);
  }
digitalWrite(VSWR_EN,LOW);
delay(3000);

    return lorawan_send(rf_vswr_port, &message[0], length);
}