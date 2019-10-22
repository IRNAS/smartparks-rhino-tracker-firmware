#ifndef BOARD_H_
#define BOARD_H_

#define VER2_2_4

#ifdef VER2_2_4
    #define LED_RED PA0
    #define VSWR_ADC PA3
    #define BAN_MON PA4
    #define BAN_MON_EN PH1
    #define VSWR_EN PA5
    #define GPS_BCK PA8
    #define GPS_EN PB6
    #define A_INT2 PB7
    #define A_INT1 PB2
    #define SEN PB12
    #define SCLK PB13
    #define LIGHT_EN PB14
    #define SDAT PB15
    #define PIN_REED PH0
#endif

#ifdef VER2_2
    #define PIN_REED PH0
    #define GPS_EN PB14
    #define LIGHT_EN PB15
    #define BAN_MON_EN PB12
    #define BAN_MON PA4
    #define GPS_BCK PA8
    #define PIN_ADC PA5
    #define LED_RED PA0
    #define A_INT1 PB2
    #define A_INT2 PB7
    #define VSWR_EN PB13
    #define VSWR_ADC PA3
#endif

#ifdef VER2_1
    #define PIN_REED PA11
    #define GPS_EN PB14
    #define LIGHT_EN PB13
    #define BAN_MON_EN PB12
    #define BAN_MON PA4
    #define GPS_BCK PA8
    #define PIN_ADC PA5
    #define LED_RED PA0
    #define A_INT1 PB5
    #define A_INT2 PB6
    #define VSWR_EN -1
    #define VSWR_ADC -1
#endif

#endif