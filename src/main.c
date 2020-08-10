#include <stdio.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "leds.h"
#include "serial.h"
#include "thermometer.h"
#include "modem.h"
#include "millis.h"

//

// tmp
extern uint8_t MODEM_BUF[MODEM_BUF_SIZE];
extern size_t MODEM_BUF_IDX;

////

static void main_setup(void) {

    rcc_clock_setup_hsi(&rcc_clock_config[2]);  // 16mhz hsi raw

    // some debug signals PA10 and PB3
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO10);
    gpio_clear(GPIOA, GPIO10);
    rcc_periph_clock_enable(RCC_GPIOB);
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3);
    gpio_clear(GPIOB, GPIO3);

}

int main(void) {

    float temp;

    main_setup();
    millis_setup();
    leds_setup();
    serial_setup();
    thermometer_setup();
    modem_setup();
    setbuf(stdout, NULL);   // optional

    printf("\n[STATUS] sciota is risen\n");

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

    if (!modem_get_imei()) {
        printf("[ERROR] modem_get_imei failed\n");
        while (1);
    }
    printf("[STATUS] IMEI: %s\n", modem_imei_str());

    if (!modem_gps_enable()) {
        printf("[ERROR] modem_gps_enable failed\n");
        while (1);
    }
    printf("[STATUS] GPS is up\n");

    // main loop

    printf("\n[STATUS] entering main loop\n");

    uint8_t rssi, ber, netstat;

    while (1) {

        printf("\n");

        // blink the LED
        leds_green_on();
        millis_delay(500);
        leds_green_off();
        millis_delay(500);

        // print time
        printf("Time (s): %.3f\n", millis()/1000.);

        // print temp
        temp = thermometer_read();
        printf("Temp (C): %.3f\n", temp);

        // print RSSI
        if (!modem_get_rssi_ber(&rssi, &ber)) {
            printf("[ERROR] modem_get_rssi_ber failed\n");
        } else {
            printf("RSSI = %d, BER = %d\n", rssi, ber);
        }

        // print network registration status
        if (!modem_get_network_registration(&netstat)) {
            printf("[ERROR] modem_get_network_registration failed\n");
        } else {
            printf("Network status: %d\n", netstat);
        }

        // GPS test
        if (!modem_gps_get_nav()) {
            printf("[ERROR] modem_gps_get_location failed\n");
        } else {
            printf("GPS: %s\n", (char*) modem_get_buffer());
        }
        
        
    }

    return 0;

}

