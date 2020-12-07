#ifndef BOARD_H_
#define BOARD_H_

#define FW_VERSION 0x0201 // upper byte is major, lower is minor

#define IFREMER

#ifdef IFREMER
    // Defines needed for water sensors
    #define BOOST_EN  PB6
    #define UART_INH  -1
    #define UART_SEL_A  -1
    #define UART_SEL_B  -1
    #define DRIVER_EN  PA3
    #define ENABLE_DRIVER  LOW
    #define DISABLE_DRIVER  HIGH
    #define LED_RED PH0

    //#define VSWR_ADC -1
    //#define BAT_MON PA4
    //#define BAT_MON_EN PH1 // reused for SD
    //#define PULSE_OUT -1
    //#define PULSE_IN -1
    //#define INPUT_AN PA5
    float static input_calib[]={
        2.016,
        2.016,
        1.5685,
        1.413666667,
        1.32975,
        1.28,
        1.245,
        1.22,
        1.20125,
        1.186666667,
        1.175,
        1.165454545,
        1.156666667,
        1.15,
        1.143571429,
        1.138666667,
        1.133125,
        1.129411765,
        1.125555556,
        1.122105263,
        1.1195,
        1.116666667,
        1.113636364,
        1.11173913,
        1.109583333,
        1.1072,
        1.105384615,
        1.102962963,
        1.101428571,
        1.10215353,
        1.100928681,
        1.099782854
    };

    //#define GPS_BCK PA8
    //#define CHG_DISABLE PA11
    //#define VSWR_EN PB5
    #define GPS_EN PB5
    //#define A_INT2 PB2
    //#define A_INT1 PB7
    //#define LIGHT_EN PB14 // reused for SD card
    //#define PIN_REED PH0
    #define BAT_MON_CALIB 1.14
#endif



// lion
#ifdef VER2_4_CAMERA
    #define LED_RED PA0
    #define VSWR_ADC PA3
    //#define BAT_MON PA4
    //#define BAT_MON_EN PH1 // reused for SD
    #define PULSE_OUT PH1
    #define PULSE_IN PB14 
    #define INPUT_AN PA5
    float static input_calib[]={
        2.016,
        2.016,
        1.5685,
        1.413666667,
        1.32975,
        1.28,
        1.245,
        1.22,
        1.20125,
        1.186666667,
        1.175,
        1.165454545,
        1.156666667,
        1.15,
        1.143571429,
        1.138666667,
        1.133125,
        1.129411765,
        1.125555556,
        1.122105263,
        1.1195,
        1.116666667,
        1.113636364,
        1.11173913,
        1.109583333,
        1.1072,
        1.105384615,
        1.102962963,
        1.101428571,
        1.10215353,
        1.100928681,
        1.099782854
    };

    
    #define GPS_BCK PA8
    #define CHG_DISABLE PA11
    #define VSWR_EN PB5
    #define GPS_EN PB6
    #define A_INT2 PB2
    #define A_INT1 PB7
    //#define LIGHT_EN PB14 // reused for SD card
    #define PIN_REED PH0
    #define BAT_MON_CALIB 1.14
#endif


// dropoff
#ifdef VER2_3_DROPOFF
    #define LED_RED PA0
    #define VSWR_ADC PA3
    #define BAT_MON PA4
    #define BAT_MON_EN PH1
    #define INPUT_AN PA5
    #define DROP_CHG PB14 
    #define DROP_EN PA0
    float static input_calib[]={
        2.016,
        2.016,
        1.5685,
        1.413666667,
        1.32975,
        1.28,
        1.245,
        1.22,
        1.20125,
        1.186666667,
        1.175,
        1.165454545,
        1.156666667,
        1.15,
        1.143571429,
        1.138666667,
        1.133125,
        1.129411765,
        1.125555556,
        1.122105263,
        1.1195,
        1.116666667,
        1.113636364,
        1.11173913,
        1.109583333,
        1.1072,
        1.105384615,
        1.102962963,
        1.101428571,
        1.10215353,
        1.100928681,
        1.099782854
    };

    #define GPS_BCK PA8
    #define CHG_DISABLE PA11
    #define VSWR_EN PB5
    #define GPS_EN PB6
    #define A_INT2 PB2
    #define A_INT1 PB7
    //#define LIGHT_EN PB14 // NOT USED
    #define PIN_REED PH0
    #define BAT_MON_CALIB 1.14
#endif

// lion
#ifdef VER2_3_FENCE
    #define LED_RED PA0
    #define VSWR_ADC PA3
    #define BAT_MON PA4
    #define BAT_MON_EN PH1
    #define INPUT_AN PA5
    #define ADS_EN PB14 // for ADS sensor
    #define ADS_ADC PA3 // analog pin used for fence monitoring
    float static input_calib[]={
        2.016,
        2.016,
        1.5685,
        1.413666667,
        1.32975,
        1.28,
        1.245,
        1.22,
        1.20125,
        1.186666667,
        1.175,
        1.165454545,
        1.156666667,
        1.15,
        1.143571429,
        1.138666667,
        1.133125,
        1.129411765,
        1.125555556,
        1.122105263,
        1.1195,
        1.116666667,
        1.113636364,
        1.11173913,
        1.109583333,
        1.1072,
        1.105384615,
        1.102962963,
        1.101428571,
        1.10215353,
        1.100928681,
        1.099782854
    };

    
    #define GPS_BCK PA8
    #define CHG_DISABLE PA11
    #define VSWR_EN PB5
    #define GPS_EN PB6
    #define A_INT2 PB2
    #define A_INT1 PB7
    //#define LIGHT_EN PB14 // NOT USED
    #define PIN_REED PH0
    #define BAT_MON_CALIB 1.14
#endif

// lion
#ifdef VER2_3_LION
    #define LED_RED PA0
    #define VSWR_ADC PA3
    #define BAT_MON PA4
    #define BAT_MON_EN PH1
    #define INPUT_AN PA5
    //#define ADS_EN PB14 // for ADS sensor
    //#define ADS_ADC PA3 // analog pin used for fence monitoring
    float static input_calib[]={
        2.016,
        2.016,
        1.5685,
        1.413666667,
        1.32975,
        1.28,
        1.245,
        1.22,
        1.20125,
        1.186666667,
        1.175,
        1.165454545,
        1.156666667,
        1.15,
        1.143571429,
        1.138666667,
        1.133125,
        1.129411765,
        1.125555556,
        1.122105263,
        1.1195,
        1.116666667,
        1.113636364,
        1.11173913,
        1.109583333,
        1.1072,
        1.105384615,
        1.102962963,
        1.101428571,
        1.10215353,
        1.100928681,
        1.099782854
    };

    #define GPS_BCK PA8
    #define CHG_DISABLE PA11
    #define VSWR_EN PB5
    #define GPS_EN PB6
    #define A_INT2 PB2
    #define A_INT1 PB7
    //#define LIGHT_EN PB14 // NOT USED
    #define PIN_REED PH0
    #define BAT_MON_CALIB 1.14
#endif

#ifdef VER2_2_4_RHINO
    #define LED_RED PA0
    #define VSWR_ADC PA3
    #define BAT_MON PA4
    #define BAT_MON_EN PH1
    #define VSWR_EN PA5
    #define GPS_BCK PA8
    #define GPS_EN PB6
    #define A_INT2 PB7
    #define A_INT1 PB2
    #define LIGHT_EN PB14
    #define PIN_REED PH0
    #define BAT_MON_CALIB 1
#endif

#ifdef VER2_2
    #define PIN_REED PH0
    #define GPS_EN PB14
    #define LIGHT_EN PB15
    #define BAT_MON_EN PB12
    #define BAT_MON PA4
    #define GPS_BCK PA8
    #define PIN_ADC PA5
    #define LED_RED PA0
    #define A_INT1 PB2
    #define A_INT2 PB7
    #define VSWR_EN PB13
    #define VSWR_ADC PA3
    #define BAT_MON_CALIB 1
#endif

#ifdef VER2_1
    #define PIN_REED PA11
    #define GPS_EN PB14
    #define LIGHT_EN PB13
    #define BAT_MON_EN PB12
    #define BAT_MON PA4
    #define GPS_BCK PA8
    #define PIN_ADC PA5
    #define LED_RED PA0
    #define A_INT1 PB5
    #define A_INT2 PB6
    #define VSWR_EN -1
    #define VSWR_ADC -1
    #define BAT_MON_CALIB 1
#endif

#endif
