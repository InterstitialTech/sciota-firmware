#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#include "modem.h"

static void _send_command(const char*);
static bool _confirm_response(const char*);
static bool _send_confirm(const char*, const char*);

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

    _send_confirm("ATE0", "OK");    // disable echo

}


//// static functions


static void _send_command(const char *cmd) {

    // this handles \r's and \n', no need to include them in the argument
    // TODO: flush usart buffers?

    while (*cmd) {
        usart_send_blocking(USART2, *cmd);
        cmd++;
    }

    usart_send_blocking(USART2, '\r');
    usart_send_blocking(USART2, '\n');

}

static bool _confirm_response(const char *resp) {

    // this handles \r's and \n', no need to include them in the argument
    // TODO: flush usart buffers?

    if(usart_recv_blocking(USART2) != '\r') return false;
    if(usart_recv_blocking(USART2) != '\n') return false;

    while (*resp) {
        if(usart_recv_blocking(USART2) != *resp) return false;
        resp++;
    }

    if(usart_recv_blocking(USART2) != '\r') return false;
    if(usart_recv_blocking(USART2) != '\n') return false;

    return true;

}

static bool _send_confirm(const char *cmd, const char *resp) {

    // (convenience function)

    _send_command(cmd);

    return _confirm_response(resp);

}

