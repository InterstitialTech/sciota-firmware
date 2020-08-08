#include <stdio.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#include "modem.h"
#include "millis.h"

static void _send_command(const char*);
static bool _wait_for_char(const char, uint64_t);
static bool _confirm_response(const char*, uint64_t);
static bool _send_confirm(const char*, const char*, uint64_t);

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

    modem_reset();

    // need to wait some after reset before the modem becomes responsive
    modem_wait_until_ready(5000);

    _send_confirm("ATE0", "OK", 100);    // disable echo

}

bool modem_wait_until_ready(uint64_t timeout) {

    // ideally we would just poll the STATUS line, but it is not broken out
    // so instead we send query ATE0 until we get a response (or timeout)

    uint64_t until;

    until = millis() + timeout;

    while(millis() < until) {
        if (_send_confirm("ATE0", "OK", 100)) return true;
    }

    return false;

}

void modem_power_up(void) {

    // pulse low for 100 ms
    gpio_clear(GPIOB, GPIO10);
    millis_delay(100);
    gpio_set(GPIOB, GPIO10);

    modem_wait_until_ready(4500);

}

void modem_power_down(void) {

    // pulse low for 1200 ms
    gpio_clear(GPIOB, GPIO10);
    millis_delay(1200);
    gpio_set(GPIOB, GPIO10);

    modem_wait_until_ready(6900);

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

static bool _wait_for_char(const char c, uint64_t timeout) {

    // block until char c is received (return true)
    // or timeout occurs (return false)
    // or something other than c is received (return false)

    uint64_t until;
    char r; // received

    until = millis() + timeout;

    while (1) {

        if (millis() >= until) {
            return false;   // timeout
        }

        if (USART_SR(USART2) & USART_SR_RXNE) {
            r = usart_recv(USART2);
            return r == c;
        }

    }

}

static bool _confirm_response(const char *resp, uint64_t timeout) {

    // handles \r and \n, no need to include them in the argument
    // timeout applies to first character only,
    // all subsequent characters subject to MODEM_CTO_MS

    // TODO: flush usart buffers?

    if (!_wait_for_char('\r', timeout)) return false;
    if (!_wait_for_char('\n', MODEM_CTO_MS)) return false;

    while (*resp) {
        if (!_wait_for_char(*resp, MODEM_CTO_MS)) return false;
        resp++;
    }

    if (!_wait_for_char('\r', MODEM_CTO_MS)) return false;
    if (!_wait_for_char('\n', MODEM_CTO_MS)) return false;


    return true;

}

static bool _send_confirm(const char *cmd, const char *resp, uint64_t timeout) {

    // convenience function

    _send_command(cmd);

    return _confirm_response(resp, timeout);

}

