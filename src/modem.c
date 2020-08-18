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

// forward declarations
static void _send_command(const char*);
static bool _wait_for_char(const char, uint64_t);
static bool _get_byte(uint8_t*, uint64_t);
static bool _confirm_response(const char*, uint64_t);
static bool _send_confirm(const char*, const char*, uint64_t);
static bool _get_data(size_t, uint64_t);
static bool _get_variable_length_response(uint64_t);
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

    // TODO figure out these preferred modes

    // preferred mode
    //if (!_send_confirm("AT+CNMP=2", "OK", 1000)) return false;  // automatic
    //if (!_send_confirm("AT+CNMP=13", "OK", 1000)) return false; // GSM only
    if (!_send_confirm("AT+CNMP=38", "OK", 1000)) return false; // LTE only
    //if (!_send_confirm("AT+CNMP=51", "OK", 1000)) return false; // GSM and LTE only

    // preferred selection between cat-m and nb-iot
    if (!_send_confirm("AT+CMNB=1", "OK", 1000)) return false;  // cat-M
    //if (!_send_confirm("AT+CMNB=2", "OK", 1000)) return false;  // NB-IOT
    //if (!_send_confirm("AT+CMNB=3", "OK", 1000)) return false;  // cat-M and NB-IOT

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

bool modem_get_imsi(void) {

    // result is stored in MODEM_BUF

    _send_command("AT+CIMI");

    if (!_get_data(15, 1000)) return false;
    if (!_confirm_response("OK", 1000)) return false;

    MODEM_BUF[15] = '\0';  // null term

    return true;

}

bool modem_get_imei(void) {

    // result is stored in MODEM_BUF

    _send_command("AT+GSN");

    if (!_get_data(15, 1000)) return false;
    if (!_confirm_response("OK", 1000)) return false;

    MODEM_BUF[15] = '\0';  // null term

    return true;

}

bool modem_get_firmware_version(void) {

    _send_command("AT+CGMR");

    if (!_get_data(24, 1000)) return false;
    if (!_confirm_response("OK", 1000)) return false;

    MODEM_BUF[24] = '\0';  // null term

    return true;

}

uint8_t *modem_get_buffer_data(void) {

    return MODEM_BUF;

}

char *modem_get_buffer_string(void) {

    return (char *) MODEM_BUF;

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

    if (!_confirm_response("OK", 1000)) return false;

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
    if (!_confirm_response("OK", 1000)) return false;

    // validate message
    if (strncmp((const char*)MODEM_BUF, "+CGREG: ", 8)) return false;
    if (MODEM_BUF[9] != ',') return false;

    // extract data
    *netstat = MODEM_BUF[10] - '0';

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

bool modem_get_available_networks(void) {

    // after calling, retrieve the result with modem_get_buffer();

    // NOTE: this will occasionally fail and cause the modem to shut down for
    // 3 minutes (consistently)

    _send_command("AT+COPS=?");
    _get_variable_length_response(5000);

    return _confirm_response("OK", 1000);

}


//// GPS


bool modem_gps_enable(void) {

    return _send_confirm("AT+CGNSPWR=1", "OK", 1000);

}

bool modem_gps_get_nav(void) {

    // upon success, nav info is stored in MODEM_BUF

    _send_command("AT+CGNSINF");    // nav info parsed from NMEA sentence

    if (!_get_variable_length_response(1000)) return false;
    if (!_confirm_response("OK", 1000)) return false;

    return true;
}


//// internets


bool modem_connect_bearer(void) {

    // attach service
    if (!_send_confirm("AT+CGATT=1", "OK", 1000)) return false;
    if (!_send_confirm("AT+CSTT=\"soracom.io\",\"sora\",\"sora\"", "OK", 1000)) return false;
    if (!_send_confirm("AT+CIICR", "OK", 1000)) return false;

    // IP bearer
    if (!_send_confirm("AT+SAPBR=3,1,\"APN\",\"soracom.io\"", "OK", 1000))  return false;
    if (!_send_confirm("AT+SAPBR=3,1,\"USER\",\"sora\"", "OK", 1000)) return false;
    if (!_send_confirm("AT+SAPBR=3,1,\"PWD\",\"sora\"", "OK", 1000)) return false;
    if (!_send_confirm("AT+SAPBR=1,1", "OK", 1000)) return false;

    // this will provide your IP address, if desired
    //if (!_send_confirm("AT+CIFSR", "OK", 1000)) return false;

    return true;

}

bool modem_post_temperature(float temp) {

    // HTTP POST

    char ATstring[30];
    char payload[30];

    if (!_send_confirm("AT+HTTPINIT", "OK", 1000)) return false;

    if (!_send_confirm("AT+HTTPPARA=\"CID\",1", "OK", 1000)) return false;
    if (!_send_confirm("AT+HTTPPARA=\"URL\",\"http://demo.thingsboard.io/api/v1/K11HoE3QMPE7rSHPf3Hj/telemetry\"", "OK", 1000)) return false;

    sprintf(payload, "{\"temperature\": %.3f}", temp);
    sprintf(ATstring, "AT+HTTPDATA=%d,10000", strlen(payload));  // do i need *any* delay here?
    if (!_send_confirm(ATstring, "DOWNLOAD", 5000)) return false;
    millis_delay(1000); // how short can this be?
    if (!_send_confirm(payload, "OK", 5000)) return false;

    if (!_send_confirm("AT+HTTPACTION=1", "OK", 1000)) return false;
    if (!_confirm_response("+HTTPACTION: 1,200,0", 2000)) return false;

    if (!_send_confirm("AT+HTTPTERM", "OK", 1000)) return false;

    return true;

}

bool modem_query_bearer(void) {

    // result is stored in MODEM_BUF

    // response:
    //  +SAPBR: <CID>,<status>,<ip_addr>
    // where status =
    //  0 Bearer is connecting
    //  1 Bearer is connected
    //  2 Bearer is closing
    //  3 Bearer is closed
    // and ip_addr is the address of the *bearer*

    _send_command("AT+SAPBR=2,1");
    if(!_get_variable_length_response(1000)) return false;
    if(!_confirm_response("OK", 1000)) return false;

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

    // does NOT add a null termination, because it could be used for binary
    // data as well as strings

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

static bool _get_variable_length_response(uint64_t timeout) {

    // result is stored in MODEM_BUF

    uint8_t byte;

    if (!_wait_for_char('\r', timeout)) return false;
    if (!_wait_for_char('\n', MODEM_CTO_MS)) return false;

    MODEM_BUF_IDX = 0;
    while (1) {

        if (!_get_byte(&byte, MODEM_CTO_MS)) return false;

        if (byte == '\r') {
            if (!_get_byte(&byte, MODEM_CTO_MS)) return false;
            if (byte == '\n') {
                break;
            } else {
                return false;
            }
        }

        MODEM_BUF[MODEM_BUF_IDX++] = byte;
        if (MODEM_BUF_IDX == (MODEM_BUF_SIZE - 1)) break; // -1 for null term

    }

    MODEM_BUF[MODEM_BUF_IDX++] = '\0';  // null term

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

