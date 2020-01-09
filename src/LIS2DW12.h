#ifndef LIS2DW12_H
#define LIS2DW12_H

#include <Wire.h>
#include "Arduino.h"

#define LIS2DW12_ADDR                        (0x19U)
#define LIS2DW12_WHO_AM_I                    (0x0FU)
#define LIS2DW12_CTRL1                       (0x20U)
#define LIS2DW12_CTRL2                       (0x21U)
#define LIS2DW12_CTRL3                       (0x22U)
#define LIS2DW12_CTRL4_INT1_PAD_CTRL         (0x23U)
#define LIS2DW12_CTRL6                       (0x25U)
#define LIS2DW12_OUT_T                       (0x26U)
#define LIS2DW12_STATUS                      (0x27U)
#define LIS2DW12_OUT_X_L                     (0x28U)
#define LIS2DW12_OUT_X_H                     (0x29U)
#define LIS2DW12_OUT_Y_L                     (0x2AU)
#define LIS2DW12_OUT_Y_H                     (0x2BU)
#define LIS2DW12_OUT_Z_L                     (0x2CU)
#define LIS2DW12_OUT_Z_H                     (0x2DU)
#define LIS2DW12_TAP_THS_X                   (0x30U)
#define LIS2DW12_TAP_THS_Y                   (0x31U)
#define LIS2DW12_TAP_THS_Z                   (0x32U)
#define LIS2DW12_INT_DUR                     (0x33U)
#define LIS2DW12_WAKE_UP_THS                 (0x34U)
#define LIS2DW12_WAKE_UP_DUR                 (0x35U)
#define LIS2DW12_FREE_FALL                   (0x36U)
#define LIS2DW12_STATUS_DUP                  (0x37U)
#define LIS2DW12_WAKE_UP_SRC                 (0x38U)
#define LIS2DW12_TAP_SRC                     (0x39U)
#define LIS2DW12_ALL_INT_SRC                 (0x3BU)
#define LIS2DW12_CTRL7                       (0x3FU)

#define LIS2DW12_WU_AI_BIT                   (0x01 << 6)
#define LIS2DW12_DOUBLE_TAP_BIT              (0x01 << 4)
#define LIS2DW12_SINGLE_TAP_BIT              (0x01 << 3)
#define LIS2DW12_FF_AI_BIT                   (0x01 << 1)
#define LIS2DW12_DRDY_AI_BIT                 (0x01 << 0)

enum Event { WAKEUP, DOUBLE_TAP, SINGLE_TAP, FREE_FALL, NO_EVENT};
struct accel_data {
    float x_axis;
    float y_axis;
    float z_axis;
};


class LIS2DW12CLASS
{
    private:
        TwoWire *wire;
        void write_reg(uint8_t reg, uint8_t val);
        //uint8_t read_reg(uint8_t reg);
        int16_t twos_comp_to_signed_int(uint16_t x);
        float raw_to_mg_2g_range(int16_t x);

    public:
        LIS2DW12CLASS(TwoWire *twi=&Wire);
        bool begin();
        void free_fall_setup();
        void single_double_tap_setup();
        void single_read_setup();
        void wake_up_setup();
        void wake_up_free_fall_setup(uint8_t wake_up_thr, uint8_t wake_up_dur,uint8_t free_fall);

        accel_data read_accel_values();
        uint8_t read_temp(); 
        Event event_detection();
        uint8_t read_reg(uint8_t reg);

};

#endif /* LIS2DW12_H */
/*** end of file ***/
