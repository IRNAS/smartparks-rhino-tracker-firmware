#include "LIS2DW12.h"

LIS2DW12CLASS::LIS2DW12CLASS(TwoWire *twi)
{
    // variable wire becomes pointer to already initialized TwoWire object
    wire = twi; 
}

/* 
 * @brief Function checks WHO_AM_I register and checks its value.
 *
 * @note should be called before all other functions to check if LIS is
 *       connected.
 *
 * @return true if output of read_reg is expected value 
 */
bool LIS2DW12CLASS::begin()
{
    Wire.begin();
    Wire.setClockLowTimeout(100);
    
    // Comparing with expected value
    if(0x44 == read_reg(LIS2DW12_WHO_AM_I))
    {
        return true;
    }
    else
    {
        return false;
    }
}

/* 
 * @brief Setup free-fall detection 
 *
 * @note It seems that it has to be called
 * everytime after free_fall is detected.
 */
void LIS2DW12CLASS::free_fall_setup()
{
    // Soft-reset, reset all control registers
    write_reg(LIS2DW12_CTRL2, 0x40);

    // Set full-scale to +/-2g, this scale should be used with free-fall
    write_reg(LIS2DW12_CTRL6, 0x00);

    // Enable free fall on INT1
    write_reg(LIS2DW12_CTRL4_INT1_PAD_CTRL, 0x10);
    
    /** Following step needs more explanation. In free fall measured 
    * acceleratioin all axes goes to zero.
    *
    * With THRESHOLD we are defining amplitude on all three axes. Acceleration
    * on all three axes has to be smaller than the threshold to generate an 
    * interrupt.
    * We are setting THRESHOLD value with [2:0] bits in FREE_FALL register.
    * With DURATION we are setting minimum time needed for which the 
    * accelerations measured on all axes must be smaller than the THRESHOLD 
    * value. It depends on the ODR selected 1LSB equals 1/ODR.
    * It is set with [7:3] bits in FREE_FALL register and with [7] bit in 
    * WAKE_UP_DUR register.
    */
    
    // Current setup for 100Hz ODR: interrupt should trigger in fall of 10cm 
    write_reg(LIS2DW12_WAKE_UP_DUR, 0x00);

    write_reg(LIS2DW12_FREE_FALL, 0x1B);
    
    // Set ODR 100Hz, Low power mode 1
    write_reg(LIS2DW12_CTRL1, 0x50);

    //Enable intterupts
    write_reg(LIS2DW12_CTRL7, 0x20);
}

/* 
 * @brief Setup single and double tap detection 
 *
 */
void LIS2DW12CLASS::single_double_tap_setup()
{
    // Soft-reset, reset all control registers
    write_reg(LIS2DW12_CTRL2, 0x40);

    // Set Full-scale to +/-2g, Low-noise enabled 
    // 4g can also be used for stronger shocks, low-noise is recommended.
    write_reg(LIS2DW12_CTRL6, 0x04);

    // Enable single and double tap interrupts
    write_reg(LIS2DW12_CTRL4_INT1_PAD_CTRL, 0x48);

    // Set threshold on X
    write_reg(LIS2DW12_TAP_THS_X, 0x09);

    // Set threshold on Y and default equal priority for all axes
    write_reg(LIS2DW12_TAP_THS_Y, 0x09);

    // Set threshold on Z and enable all axes in tap recognition
    write_reg(LIS2DW12_TAP_THS_Z, 0xe9);
    
    // Explanation how to set up following register is found in application
    // note DT0101.
    // Set shock, latency and quiet duration 
    write_reg(LIS2DW12_INT_DUR, 0x7f);

    // Enable single AND double-tap recognition 
    write_reg(LIS2DW12_WAKE_UP_THS, 0x80);

    // Start up the sensor
    //Set ODR 400Hz, High performance 14 bit
    write_reg(LIS2DW12_CTRL1, 0x74);

    delay(18);  // Suggested settling time 7/ODR 
    
    //Enable interrupts
    write_reg(LIS2DW12_CTRL7, 0x20);
}

/* 
 * @brief Setup reading of X, Y and Z axis 
 *
 */
void LIS2DW12CLASS::single_read_setup()
{
    // Soft-reset, reset all control registers
    write_reg(LIS2DW12_CTRL2, 0x40);

    // Enable Block data unit which prevents continus updates of 
    // lower and upper registers. 
    write_reg(LIS2DW12_CTRL2, 0x08);

    // Set Full-scale to +/-2g
    write_reg(LIS2DW12_CTRL6, 0x00);

    // Enable single data conversion
    write_reg(LIS2DW12_CTRL3, 0x02);

    // Data-ready routed to INT1
    // write_reg(LIS2DW12_CTRL4_INT1_PAD_CTRL, 0x01);

    // Start up the sensor
    //Set ODR 50Hz, Single data mode, 14 bit resolution 
    write_reg(LIS2DW12_CTRL1, 0x49);
    
    // Settling time
    delay(20);
    
    //Enable interrupts
    write_reg(LIS2DW12_CTRL7, 0x20);
}

/* 
 * @brief Setup wakeup detection 
 *
 */
void LIS2DW12CLASS::wake_up_setup()
{
    // Soft-reset, reset all control registers
    write_reg(LIS2DW12_CTRL2, 0x40);

    //Enable BDU
    write_reg(LIS2DW12_CTRL2, 0x08);    

    //Set full scale +- 2g 
    write_reg(LIS2DW12_CTRL6, 0x00);    

    //Enable activity detection interrupt
    write_reg(LIS2DW12_CTRL4_INT1_PAD_CTRL, 0x20);    

    /**The wake-up can be personalized by two key parameters � threshold 
     * and duration. 
     * With threshold, the application can set the value which at least one 
     * of the axes has to exceed in order to generate an interrupt.
     * The duration is the number of consecutive samples for which 
     * the measured acceleration exceeds the threshold.
     */    
    write_reg(LIS2DW12_WAKE_UP_THS, 0x01);    
    write_reg(LIS2DW12_WAKE_UP_DUR, 0x00);    

    //Start sensor with ODR 100Hz and in low-power mode 1 
    write_reg(LIS2DW12_CTRL1, 0x10);    

    delay(10); //Settling time

    //Enable interrupt  function
    write_reg(LIS2DW12_CTRL7, 0x20);    
}

/* 
 * @brief Setup wakeup and free fall detection 
 *
 */
void LIS2DW12CLASS::wake_up_free_fall_setup(uint8_t wake_up_thr, uint8_t wake_up_dur,uint8_t free_fall)
{
    // Soft-reset, reset all control registers
    write_reg(LIS2DW12_CTRL2, 0x40);

    //Enable BDU
    write_reg(LIS2DW12_CTRL2, 0x08);    

    //Set full scale +- 2g 
    write_reg(LIS2DW12_CTRL6, 0x00);    

    //Enable wakeup and free fall detection interrupt
    write_reg(LIS2DW12_CTRL4_INT1_PAD_CTRL, 0x30);    

    // Programmed for 30 mm fall at 100Hz ODR
    write_reg(LIS2DW12_FREE_FALL, free_fall);
    write_reg(LIS2DW12_WAKE_UP_THS, wake_up_thr);    
    write_reg(LIS2DW12_WAKE_UP_DUR, wake_up_dur);    

    delay(100); //Settling time

    //Start sensor with ODR 100Hz and in low-power mode 1 
    write_reg(LIS2DW12_CTRL1, 0x10);    

    //Enable interrupt function
    write_reg(LIS2DW12_CTRL7, 0x20);    


}

/* 
 * @brief Setup wakeup detection 
 *
 */
accel_data LIS2DW12CLASS::read_accel_values()
{
    // Demand data, after that we wait for DATA ready bit 
    write_reg(LIS2DW12_CTRL3, 0x03);

    for(int i=0;i<100;i++)
    {
        if(read_reg(LIS2DW12_STATUS) & LIS2DW12_DRDY_AI_BIT) 
        {
            // Data is ready
            break;
        }
        delay(10);
    }


    accel_data data;
    uint8_t msb;
    uint8_t lsb;

    // For each axis read lower and upper register, concentate them,
    // convert them into signed decimal notation and map them to
    // appropriate range.
    // By default register address should be incremented automaticaly,
    // this is controled in CTRL2 but for some reason doesn't look like
    // it does.
    lsb = read_reg(LIS2DW12_OUT_X_L);  
    msb = read_reg(LIS2DW12_OUT_X_H);  
    data.x_axis = twos_comp_to_signed_int(((msb << 8) | lsb)); 

    lsb = read_reg(LIS2DW12_OUT_Y_L);  
    msb = read_reg(LIS2DW12_OUT_Y_H);  
    data.y_axis = twos_comp_to_signed_int(((msb << 8) | lsb)); 

    lsb = read_reg(LIS2DW12_OUT_Z_L);  
    msb = read_reg(LIS2DW12_OUT_Z_H);  
    data.z_axis = twos_comp_to_signed_int(((msb << 8) | lsb)); 

    data.x_axis = raw_to_mg_2g_range(data.x_axis); 
    data.y_axis = raw_to_mg_2g_range(data.y_axis); 
    data.z_axis = raw_to_mg_2g_range(data.z_axis); 

    return data;
}


/* 
 * @brief Detect event that triggered intterupt 
 *
 * @note Do not call in IRQ, read_reg will return garbage. 
 *
 * @return enumerated type of event
 *
 */
Event LIS2DW12CLASS::event_detection()
{
    uint8_t status = 0;
    status = read_reg(LIS2DW12_STATUS);
    if(status & LIS2DW12_DOUBLE_TAP_BIT)
    {
        return DOUBLE_TAP;
    }
    if(status & LIS2DW12_SINGLE_TAP_BIT)
    {
        return SINGLE_TAP;
    }
    if(status & LIS2DW12_FF_AI_BIT)
    {
        return FREE_FALL;
    }
    if(status & LIS2DW12_WU_AI_BIT)
    {
        return WAKEUP;
    }
    return NO_EVENT;
}

uint8_t LIS2DW12CLASS::read_temp()
{
    return read_reg(LIS2DW12_OUT_T);
}

/* 
 * @brief Writes value to register over I2C 
 * 
 * @param reg
 * @param val 
 *
 */
void LIS2DW12CLASS::write_reg(uint8_t reg, uint8_t val)
{
    for (int i = 0; i < 3; i++)
    {
        Wire.beginTransmission((uint8_t) LIS2DW12_ADDR);
        Wire.write((uint8_t) reg);
        Wire.write((uint8_t) val);
        int result = Wire.endTransmission();  
        if(result==0){
            break;
        }  
    }
}

/* 
 * @brief Reads value from register over I2C 
 * 
 * @param reg
 *
 * @return content of reg
 */
uint8_t LIS2DW12CLASS::read_reg(uint8_t reg)
{
    Wire.beginTransmission((uint8_t) LIS2DW12_ADDR);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t) LIS2DW12_ADDR, (uint8_t) 1);

    return Wire.read();
}

/* 
 *  @brief Convert from two's complement number into 
 *      signed decimal number 
 *
 *  @param x, number to be converted
 *
 *  @return converted number  
 */
int16_t LIS2DW12CLASS::twos_comp_to_signed_int(uint16_t x)
{
    if( x & (0x01 << 16))
    {
        // Number is negative, clear MSB bit
        x &= ~(0x01 << 16);
        x = -x;
        return x;
    }
    else
    {
        // Number is positive
        return x;
    }
}

/*
 *  @brief Convert raw accel values to milli G's mapped to +/- 2g range
 *
 *  @param x
 *
 *  @return mapped value
 */
float LIS2DW12CLASS::raw_to_mg_2g_range(int16_t x)
{
    return (x >> 2) * 0.244; 
}
