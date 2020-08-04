#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#include "modem.h"

static void _send_string(char*);

void modem_setup(void) {

    rcc_periph_clock_enable(RCC_USART2);

    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2 | GPIO3);
    gpio_set_af(GPIOA, GPIO_AF7, GPIO2 | GPIO3);

    usart_set_baudrate(USART2, 115200);
    usart_set_databits(USART2, 8);
    usart_set_parity(USART2, USART_PARITY_NONE);
    usart_set_stopbits(USART2, USART_CR2_STOPBITS_1);
    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
    usart_set_mode(USART2, USART_MODE_TX_RX);   // duplex

    usart_enable(USART2);

}

void modem_init(void) {

    _send_string("AT\r\n");

}

static void _send_string(char *string) {

    while (*string) {
        usart_send_blocking(USART2, *string);
        string++;
    }

}

