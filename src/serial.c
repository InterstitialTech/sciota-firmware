#include <unistd.h>
#include <errno.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#include "serial.h"

extern int _write(int, const char *, ssize_t);

void serial_setup(void) {

    rcc_periph_clock_enable(RCC_USART1);

    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);
    gpio_set_af(GPIOA, GPIO_AF7, GPIO9);

    usart_set_baudrate(USART1, 115200);
    usart_set_databits(USART1, 8);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_stopbits(USART1, USART_CR2_STOPBITS_1);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
    usart_set_mode(USART1, USART_MODE_TX);  // just output for now

    usart_enable(USART1);

}

int _write(int file, const char *ptr, ssize_t len) {

    // override the _write function, make it output to USART

    if (file != STDOUT_FILENO && file != STDERR_FILENO) {
        errno = EIO;
        return -1;
    }

    ssize_t i;
    for (i = 0; i < len; i++) {

        if (ptr[i] == '\n') {
            usart_send_blocking(USART1, '\r');
        }

        usart_send_blocking(USART1, ptr[i]);

    }

    return i;

}

