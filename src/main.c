// adapted from libopencm3-examples/examples/stm32/f4/nucleo-f411re/blink/blink.c
// LED on this board (STM32 Nucleo) is PA5

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

static void gpio_setup(void) {

    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);

}


int main(void) {

    int i;

    gpio_setup();

    while (1) {

        gpio_toggle(GPIOA, GPIO5);

        for (i = 0; i < 1000000; i++) {
            __asm__("nop");
        }

    }

    return 0;

}

