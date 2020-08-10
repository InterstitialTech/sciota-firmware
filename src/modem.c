#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#include "modem.h"
#include "millis.h"

// global buffers
uint8_t MODEM_BUF[MODEM_BUF_SIZE];
size_t MODEM_BUF_IDX = 0;
char MODEM_IMEI[16];

// forward declarations
static void _send_command(const char*);
static bool _wait_for_char(const char, uint64_t);
static bool _get_byte(uint8_t*, uint64_t);
static bool _confirm_response(const char*, uint64_t);
static bool _send_confirm(const char*, const char*, uint64_t);
static bool _get_data(size_t, uint64_t);
static void _flush_rx(void);

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

    // set GPIO pins
    gpio_set(GPIOB, GPIO10);  // PWRKEY high
    gpio_set(GPIOA, GPIO8);     // NRESET high
    gpio_clear(GPIOA, GPIO9);   // DTR low

}

bool modem_init(void) {

    // disable echo
    if (!_send_confirm("ATE0", "OK", 100)) return false;

    // TODO configure modem parameters

    return true;

}

bool modem_wait_until_ready(uint64_t timeout) {

    // ideally we would just poll the STATUS line, but it is not broken out
    // so instead we send ATE0 until we get a response (or timeout)

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

}

void modem_power_down(void) {

    // pulse low for 1200 ms
    gpio_clear(GPIOB, GPIO10);
    millis_delay(1200);
    gpio_set(GPIOB, GPIO10);

}

void modem_reset(void) {

    // pulse high for >= 252 ms
    gpio_clear(GPIOA, GPIO8);
    millis_delay(260);
    gpio_set(GPIOA, GPIO8);

}

bool modem_get_imei(void) {

    // use this to get the IMEI from the modem via AT+GSN
    // the result is stored in file-scope variable MODEM_IMEI

    _send_command("AT+GSN");

    if (!_get_data(15, 1000)) return false;

    strncpy(MODEM_IMEI, (const char *) MODEM_BUF, 15);
    MODEM_IMEI[15] = '\0';

    return true;

}

char *modem_imei_str(void) {

    // use this to access MODEM_IMEI from outside this module

    return MODEM_IMEI;

}

bool modem_get_firmware_version(void) {

    _send_command("AT+CGMR");

    if (!_get_data(24, 1000)) return false;
    if (!_confirm_response("OK", 1000)) return false;

    MODEM_BUF[MODEM_BUF_IDX++] = '\0';

    return true;

}

uint8_t *modem_get_buffer(void) {

    return MODEM_BUF;

}

bool modem_get_rssi_ber(uint8_t *rssi, uint8_t *ber) { 

    // RSSI is a encoded as a number from 0 to 31 (inclusive) representing dBm
    // from -115 to -52.
    // BER (bit error rate) is encoded as a number from 0 to 7 (inclusive),
    // representing "RxQual" (https://en.wikipedia.org/wiki/Rxqual).
    // Both values will rail to 99 if they are "not known or detectable".
    // We return these numbers as a uint8_t's via the output parameters

    uint8_t result;

    _send_command("AT+CSQ");

    if (!_get_data(11, 1000)) return false;
    MODEM_BUF[MODEM_BUF_IDX++] = '\0';  // add null termination for strtol()

    // validate message
    if (strncmp((const char*)MODEM_BUF, "+CSQ:", 5)) return false;
    if (MODEM_BUF[8] != ',') return false;

    // extract data
    *rssi = strtol((const char*) (MODEM_BUF + 5), NULL, 10);
    *ber = strtol((const char*) (MODEM_BUF + 9), NULL, 10);

    return true;

}

bool modem_get_network_registration(uint8_t *netstat) {

    _send_command("AT+CGREG?");

    if (!_get_data(11, 1000)) return false;

    // validate message
    if (strncmp((const char*)MODEM_BUF, "+CGREG: ", 8)) return false;
    if (MODEM_BUF[9] != ',') return false;

    // extract data
    *netstat = MODEM_BUF[10] - '0';

    return true;

}

bool modem_set_network_details(void) {

    _send_command("AT+CGDCONT=1,\"IP\",\"wireless.twilio.com\"");
    _confirm_response("OK", 1000);
    _confirm_response("SMS Ready", 1000);

    return true;

}

bool modem_get_network_system_mode(uint8_t *status) {

    // status =  0 (no service)
    //           1 (GSM)
    //           3 (EGPRS)
    //           7 (LTE M1)
    //           9 (LTE NB)

    _send_command("AT+CNSMOD?");

    if (!_get_data(12, 1000)) return false;
    if (!_confirm_response("OK", 1000)) return false;

    // validate message
    if (strncmp((const char*)MODEM_BUF, "+CNSMOD: ", 9)) return false;
    if (MODEM_BUF[10] != ',') return false;

    *status = MODEM_BUF[11] - '0';

    return true;

}

bool modem_get_functionality(uint8_t *status) {

    //  status =    0 (Minimum functionality)
    //              1 (Full functionality (Default))
    //              4 (Disable phone both transmit and receive RF circuits)
    //              5 (Factory Test Mode)
    //              6 (Reset)
    //              7 (Offline Mode)

    _send_command("AT+CFUN?");

    if (!_get_data(8, 1000)) return false;
    if (!_confirm_response("OK", 1000)) return false;

    // validate message
    if (strncmp((const char*)MODEM_BUF, "+CFUN: ", 7)) return false;

    *status = MODEM_BUF[7] - '0';

    return true;

}



//// static functions


static void _send_command(const char *cmd) {

    // handles \r and \n, no need to include them in the argument

    _flush_rx();

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

static bool _get_byte(uint8_t *b, uint64_t timeout) {

    uint64_t until;

    until = millis() + timeout;

    while (1) {

        if (millis() >= until) {
            return false;   // timeout
        }

        if (USART_SR(USART2) & USART_SR_RXNE) {
            *b = usart_recv(USART2);
            return true;
        }

    }

}

static bool _get_data(size_t len, uint64_t timeout) {

    if (len > MODEM_BUF_SIZE) {
        printf("[ERROR] _get_data: len is greater than MODEM_BUF_SIZE\n");
        return false;
    }

    if (!_wait_for_char('\r', timeout)) return false;
    if (!_wait_for_char('\n', MODEM_CTO_MS)) return false;

    MODEM_BUF_IDX = 0;
    for (size_t i=0; i<len; i++) {
        if (!_get_byte(MODEM_BUF + MODEM_BUF_IDX++, MODEM_CTO_MS)) return false;
    }

    if (!_wait_for_char('\r', MODEM_CTO_MS)) return false;
    if (!_wait_for_char('\n', MODEM_CTO_MS)) return false;

    return true;

}

static bool _confirm_response(const char *resp, uint64_t timeout) {

    // handles \r and \n, no need to include them in the argument
    // timeout applies to first character only,
    // all subsequent characters subject to MODEM_CTO_MS

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

static void _flush_rx(void) {

    while (USART_SR(USART2) & USART_SR_RXNE) {
        usart_recv(USART2);
        millis_delay(2);    // give the usart at least 1 ms to set RXNE again
    }

}

