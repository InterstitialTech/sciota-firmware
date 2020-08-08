#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#include "modem.h"
#include "millis.h"

static void _send_command(const char*);
static bool _confirm_response(const char*);
static bool _send_confirm(const char*, const char*);

void modem_setup(void) {

    // enable peripheral clocks
    rcc_periph_clock_enable(RCC_USART2);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);


    // configure USART2 on PA2 (TX) and PA3 (RX)
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2 | GPIO3);
    gpio_set_af(GPIOA, GPIO_AF7, GPIO2 | GPIO3);
    usart_set_baudrate(USART2, 115200);
    usart_set_databits(USART2, 8);
    usart_set_parity(USART2, USART_PARITY_NONE);
    usart_set_stopbits(USART2, USART_CR2_STOPBITS_1);
    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
    usart_set_mode(USART2, USART_MODE_TX_RX);   // duplex
    usart_enable(USART2);

    // configure GPIO pins
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO10);   // PWRKEY
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO8);    // NRESET
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO9);    // DTR
    gpio_mode_setup(GPIOC, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO7);     // RI

}

void modem_init(void) {

    gpio_set(GPIOB, GPIO10);  // PWRKEY high
    gpio_set(GPIOA, GPIO8);     // NRESET high
    gpio_clear(GPIOA, GPIO9);   // DTR low

    // if we just powered up (or reset), then we need to wait some time before
    // the modem becomes responsive. for now, just wait about 5 seconds
    // TODO: switch over to a while loop with timeouts
    millis_delay(5000);

    _send_confirm("ATE0", "OK");    // disable echo


}

void modem_power_up(void) {

    // pulse low for 100 ms
    gpio_clear(GPIOB, GPIO10);
    millis_delay(100);
    gpio_set(GPIOB, GPIO10);

    // ideally, we would wait until the STATUS pin goes high,
    // but this pin is not broken out on the botletics board
    // so for now we'll just wait (at least 4.5 seconds)
    millis_delay(4500);

}

void modem_power_down(void) {

    // pulse low for 1200 ms
    gpio_clear(GPIOB, GPIO10);
    millis_delay(1200);
    gpio_set(GPIOB, GPIO10);

    // see comment in module_power_up()
    millis_delay(6900);

}

void modem_reset(void) {

    // pulse high for >= 252 ms
    gpio_clear(GPIOA, GPIO8);
    millis_delay(260);
    gpio_set(GPIOA, GPIO8);

}

void modem_get_imei(void) {

    _send_command("AT+GSN");
    // TODO recv

}

void modem_get_rssi(void) {

    _send_command("AT+CSQ");
    // TODO recv

}


//// static functions


static void _send_command(const char *cmd) {

    // handles \r and \n, no need to include them in the argument
    // TODO: flush usart buffers?

    while (*cmd) {
        usart_send_blocking(USART2, *cmd);
        cmd++;
    }

    usart_send_blocking(USART2, '\r');
    usart_send_blocking(USART2, '\n');

}

static bool _confirm_response(const char *resp) {

    // handles \r and \n, no need to include them in the argument
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

