#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/i2c.h>

static void setup_clock(void) {

    rcc_clock_setup_hsi(&rcc_clock_config[2]);  // 16mhz hsi raw

}

static void setup_gpio(void) {

    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);

}

static void setup_usart(void) {

    rcc_periph_clock_enable(RCC_USART1);

    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);
    gpio_set_af(GPIOA, GPIO_AF7, GPIO9);

    usart_set_baudrate(USART1, 115200);
    usart_set_databits(USART1, 8);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_stopbits(USART1, USART_CR2_STOPBITS_1);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
    usart_set_mode(USART1, USART_MODE_TX);

    usart_enable(USART1);

    setbuf(stdout, NULL);   // optional

}

static void setup_i2c(void) {

    // GPIOB init
    rcc_periph_clock_enable(RCC_GPIOB);
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO8 | GPIO9);
    gpio_set_output_options(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_10MHZ, GPIO8 | GPIO9);
    gpio_set_af(GPIOB, GPIO_AF4, GPIO8 | GPIO9);

    // I2C1 init
    rcc_periph_clock_enable(RCC_I2C1);
    rcc_periph_reset_pulse(RST_I2C1);
    i2c_set_speed(I2C1, i2c_speed_sm_100k, 16); // ?? uhh lowercase library define?
    i2c_peripheral_enable(I2C1);

}

////

static float rawTemp2float(uint8_t *rawTemp) {

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

static float mcp9808_readTemp(void) {

    uint8_t regAddr_temp = 0b0101;
    uint8_t buffer[2];

    i2c_transfer7(I2C1, 0x18, &regAddr_temp, 1, buffer, 2);

    return rawTemp2float(buffer);

}

////

extern int _write(int, const char *, ssize_t);

int _write(int file, const char *ptr, ssize_t len) {

    if (file != STDOUT_FILENO && file != STDERR_FILENO) {
        errno = EIO;
        return -1;
    }

    ssize_t i;
    for (i = 0; i < len; i++) {

        if (ptr[i] == '\n') {
            usart_send_blocking(USART1, '\r');
        }

        usart_send_blocking(USART1, ptr[i]);

    }

    return i;

}


////

int main(void) {

    int i;
    float temp;

    setup_clock();
    setup_gpio();
    setup_usart();
    setup_i2c();

    while (1) {

        // blink the LED
        gpio_set(GPIOA, GPIO5);
        for (i = 0; i < 1000000; i++) {
            __asm__("nop");
        }
        gpio_clear(GPIOA, GPIO5);
        for (i = 0; i < 1000000; i++) {
            __asm__("nop");
        }

        // read temp
        temp = mcp9808_readTemp();
        printf("temperature: %.3f\n", temp);

    }

    return 0;

}

