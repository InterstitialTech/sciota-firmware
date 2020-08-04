#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>

#include "thermometer.h"

static float _rawTemp2float(uint8_t*);

void thermometer_setup(void) {

    rcc_periph_clock_enable(RCC_GPIOB);
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO8 | GPIO9);
    gpio_set_output_options(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_10MHZ, GPIO8 | GPIO9);
    gpio_set_af(GPIOB, GPIO_AF4, GPIO8 | GPIO9);

    rcc_periph_clock_enable(RCC_I2C1);
    rcc_periph_reset_pulse(RST_I2C1);
    i2c_set_speed(I2C1, i2c_speed_sm_100k, 16);
    i2c_peripheral_enable(I2C1);

}

void thermometer_init(void) {

    // TODO

}

float thermometer_read(void) {

    uint8_t regAddr_temp = 0b0101;
    uint8_t buffer[2];

    i2c_transfer7(I2C1, 0x18, &regAddr_temp, 1, buffer, 2);

    return _rawTemp2float(buffer);

}

static float _rawTemp2float(uint8_t *rawTemp) {

    // see example 5-1 in the in the MCP9808 datasheet

    uint8_t MSB, LSB;
    float result;

    MSB = rawTemp[0];
    LSB = rawTemp[1];

    MSB = MSB & 0x1f;   // clear flag bits
    if (MSB & 0x10) {
        MSB = MSB & 0x0f;   // clear sign bit
        result = 256 - (MSB * 16 + LSB / 16.);
    } else {
        result = (MSB * 16 + LSB / 16.);
    }

    return result;
}

