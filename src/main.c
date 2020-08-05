#include <stdio.h>

#include <libopencm3/stm32/rcc.h>

#include "leds.h"
#include "serial.h"
#include "thermometer.h"
#include "modem.h"

static void main_setup(void) {

    rcc_clock_setup_hsi(&rcc_clock_config[2]);  // 16mhz hsi raw

}

int main(void) {

    int i;
    float temp;

    main_setup();
    leds_setup();
    serial_setup();
    thermometer_setup();
    modem_setup();

    setbuf(stdout, NULL);   // optional

    modem_init();

    while (1) {

        // blink the LED
        leds_green_on();
        for (i = 0; i < 1000000; i++) {
            __asm__("nop");
        }
        leds_green_off();
        for (i = 0; i < 1000000; i++) {
            __asm__("nop");
        }

        // read temp
        temp = thermometer_read();
        printf("temperature: %.3f\n", temp);

    }

    return 0;

}

