#include <stdio.h>

#include <libopencm3/stm32/rcc.h>

#include "leds.h"
#include "serial.h"
#include "thermometer.h"
#include "modem.h"
#include "millis.h"

////

static void main_setup(void) {

    rcc_clock_setup_hsi(&rcc_clock_config[2]);  // 16mhz hsi raw

}

int main(void) {

    int i;
    float temp;

    main_setup();
    millis_setup();
    leds_setup();
    serial_setup();
    thermometer_setup();
    modem_setup();
    setbuf(stdout, NULL);   // optional

    // bring up the modem
    modem_reset();
    if (!modem_wait_until_ready(10000)) {
        printf("[ERROR] modem is unresponsive\n");
        while (1);
    }
    printf("[STATUS] modem up\n");
    if (!modem_init()) {
        printf("[ERROR] modem_init failed\n");
        while (1);
    }
    printf("[STATUS] modem initialized\n");

    while (1) {

        // blink the LED
        leds_green_on();
        millis_delay(500);
        leds_green_off();
        millis_delay(500);

        // print time
        printf("millis: %d\n", (int) millis());

        // print temp
        temp = thermometer_read();
        printf("temperature: %.3f\n\n", temp);


        // query modem
        modem_get_imei();

    }

    return 0;

}

