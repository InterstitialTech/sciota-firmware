#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include "leds.h"

void leds_setup(void) {

    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);

}

void leds_green_on(void) {

    gpio_set(GPIOA, GPIO5);

}

void leds_green_off(void) {

    gpio_clear(GPIOA, GPIO5);

}
