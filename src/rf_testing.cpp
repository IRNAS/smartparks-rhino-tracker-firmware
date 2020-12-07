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

static boolean rf_scan(void);
static boolean rf_autotune(void);
static boolean scan_vswr(uint32_t start, uint32_t stop, int8_t power, uint16_t samples, uint32_t time, uint8_t *output);

/**
 * @brief Initialize the rf logic
 * 
 */
void rf_init(void){ 
    #ifdef debug
        serial_debug.print("rf_init - ( ");
        serial_debug.println(" )");
    #endif
#ifdef VSWR_EN
    pinMode(VSWR_EN,OUTPUT);
    digitalWrite(VSWR_EN,LOW);
#endif
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
#ifdef VSWR_ADC
static boolean scan_vswr(uint32_t start, uint32_t stop, int8_t power, uint16_t samples, uint32_t time, uint8_t *output)
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
        STM32L0.deepsleep(1000);
        STM32L0.wdtReset(); // just a hack due to a large delay in this loop

        digitalWrite(VSWR_EN,HIGH);
        
        while(LoRaWAN.setTxContinuousWave(freq,power,100)==false){
            delay(500);
            Serial.println("cw wait");
        }
        
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
        digitalWrite(VSWR_EN,LOW);


        double value = avg/count; // calculate average and convert to volts
        double dbm=-31; // covnert to dBm
        // secure against division by zero
        if(value!=0){
            dbm = 10*log10(value)-27;
        }
        #ifdef debug
            serial_debug.print(value); 
            serial_debug.print(" ");
            serial_debug.print(dbm); 
            serial_debug.println(" dBm) ");
        #endif
        *output=value/10;
        output++;
        //increment frequency
        freq+=step;
        if(freq>stop){
            break;
        }
    }
    return true;
}
#endif

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

    boolean status = false;

    if(rf_settings_packet.data.type==1){
#ifdef VSWR_ADC
        status= rf_autotune();
#endif
    }
    else if(rf_settings_packet.data.type==2)
    {
#ifdef VSWR_ADC
        status=rf_scan();
#endif
    }
    

    //inplementing different types of measurements
    // type=0 set tuning value
    // type=1 rf_autotune
    // type=2 vswr scan frequency
    return status;
}

#ifdef VSWR_ADC
static boolean rf_scan(){
    digitalWrite(VSWR_EN,LOW);
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
#endif

#ifdef VSWR_ADC
static boolean rf_autotune(void){

    #ifdef debug
        serial_debug.print("rf_autotune( ");
        serial_debug.println(" )");
    #endif
    float tune_start  = 0;
    float tune_stop   = 15;
    float tune_step   = 1;
    uint8_t *output = &message[0];
    uint8_t length = 0;
    float vcc = 2.5;
    pinMode(VSWR_EN,OUTPUT);
    digitalWrite(VSWR_EN,LOW);

  for(float tune=0;tune<tune_stop;tune++){
    delay(1000);
    STM32L0.wdtReset(); // just a hack due to a large delay in this loop
    DTC_Initialize(STM32L0_GPIO_PIN_PB12, tune, STM32L0_GPIO_PIN_NONE, 0b0);
    digitalWrite(VSWR_EN,HIGH);

    while(LoRaWAN.setTxContinuousWave((unsigned long)868000000,(float)5,(unsigned long)100)==0){
        delay(100);
    }
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
    digitalWrite(VSWR_EN,LOW);
    *output=avg/count/10;
    output++;
    length++;
    #ifdef debug
        serial_debug.println(avg/count);
    #endif
  }
digitalWrite(VSWR_EN,LOW);
delay(3000);

    return lorawan_send(rf_vswr_port, &message[0], length);
}
#endif
