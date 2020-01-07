#ifndef BOARD_H_
#define BOARD_H_

#define VER2_3_LION

// lion
#ifdef VER2_3_LION
    #define LED_RED PA0
    #define VSWR_ADC PA3
    #define BAT_MON PA4
    #define BAT_MON_EN PH1
    #define INPUT_AN PA5
    float static input_calib[]={
        6.820428471349072197,
        2.004329283574737275,
        1.619855259204599207,
        1.423069011076018953,
        1.346277995719406562,
        1.296903950667421945,
        1.264141095908950163,
        1.240822400722206176,
        1.219517283603282642,
        1.206887484178402126,
        1.193577346100077463,
        1.184074015175272088,
        1.174752622167649818,
        1.169524222034264582,
        1.159575131946372828,
        1.151892661635952919,
        1.154641779169629601,
        1.147920267739575451,
        1.144081142907936943,
        1.140526339150688706,
        1.136164975748118344,
        1.136493165165849817,
        1.133101425390103323,
        1.129524792356504825,
        1.127849833366320453,
        1.125464138662470859,
        1.123920476843982286,
        1.124073302623302162,
        1.117122345598519573,
        1.102695191528939489,
        1.080791840414563865
    };
    #define GPS_BCK PA8
    #define CHG_DISABLE PA11
    #define VSWR_EN PB5
    #define GPS_EN PB6
    #define A_INT2 PB2
    #define A_INT1 PB7
    #define LIGHT_EN PB14
    #define PIN_REED PH0
    #define BAT_MON_CALIB 1
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
