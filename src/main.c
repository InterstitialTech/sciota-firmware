#include <stdio.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "leds.h"
#include "serial.h"
#include "thermometer.h"
#include "modem.h"
#include "millis.h"

//

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

    main_setup();
    millis_setup();
    leds_setup();
    serial_setup();
    thermometer_setup();
    modem_setup();
    setbuf(stdout, NULL);   // optional

    printf("\n[STATUS] sciota is risen\n\n");


    // bring up the modem

    modem_reset();
    while (!modem_wait_until_ready(10000)) {
        printf("[ERROR] modem is unresponsive\n");
    }

    while (!modem_init()) {
        printf("[ERROR] modem_init failed\n");
    }

    while (!modem_get_imei()) {
        printf("[ERROR] modem_get_imei failed\n");
    }
    printf("[STATUS] IMEI: %s\n", modem_get_buffer_string());

    while (!modem_get_imsi()) {
        printf("[ERROR] modem_get_imsi failed\n");
    }
    printf("[STATUS] IMSI: %s\n", modem_get_buffer_string());

    while (!modem_get_firmware_version()) {
        printf("[ERROR] modem_get_firmware_version failed\n");
    }
    printf("firmware version = %s\n", modem_get_buffer_string());


    // main loop

    printf("\n[STATUS] entering main loop\n");

    float temp;
    uint8_t fun, rssi, ber, reg, mode;  // modem vitals

    bool ip_connected = false;

    while (1) {

        printf("\n");

        // blink the LED
        leds_green_on();
        millis_delay(500);
        leds_green_off();
        millis_delay(500);

        // get the time and temperature
        printf("Time (s): %.3f\n", millis()/1000.);
        temp = thermometer_read();
        printf("Temp (C): %.3f\n", temp);

        // modem vitals
        if (!modem_get_functionality(&fun)) {
            printf("[ERROR] modem_get_functionality failed\n");
        } else {
            printf("Modem Functionality: %d\n", fun);
        }
        if (!modem_get_rssi_ber(&rssi, &ber)) {
            printf("[ERROR] modem_get_rssi_ber failed\n");
        } else {
            printf("RSSI = %d, BER = %d\n", rssi, ber);
        }
        if (!modem_get_network_registration(&reg)) {
            printf("[ERROR] modem_get_network_registration failed\n");
        } else {
            printf("Network Registration Status: %d\n", reg);
        }
        if (!modem_get_network_system_mode(&mode)) {
            printf("[ERROR] modem_get_network_system_mode failed\n");
        } else {
            printf("Network System Mode: %d\n", mode);
        }

        // internets
        if ((fun==1) && (reg==5) && (mode==7)) {

            if (!ip_connected) {
                while (!modem_connect_bearer()) {
                    printf("[ERROR] modem_connect_bearer failed\n");
                    millis_delay(1000);
                }
                printf("Bearer connection established\n");
                ip_connected = true;
            }

            if (!modem_post_temperature(temp)) {
                printf("[ERROR] modem_post_temperature failed\n");
                ip_connected = false;
            } else {
                printf("HTTP POST succeeded\n");
            }

        }

        millis_delay(5000);

    }

    return 0;

}

