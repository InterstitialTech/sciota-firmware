#include <unistd.h>
#include <errno.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#include "serial.h"

extern int _write(int, const char *, ssize_t);

void serial_setup(void) {

    // USART3 on PC10 (TX) and PC11 (RX)

    rcc_periph_clock_enable(RCC_USART3);
    rcc_periph_clock_enable(RCC_GPIOC);

    gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO10 | GPIO11);
    gpio_set_af(GPIOC, GPIO_AF7, GPIO10 | GPIO11);

    usart_set_baudrate(USART3, 115200);
    usart_set_databits(USART3, 8);
    usart_set_parity(USART3, USART_PARITY_NONE);
    usart_set_stopbits(USART3, USART_CR2_STOPBITS_1);
    usart_set_flow_control(USART3, USART_FLOWCONTROL_NONE);
    usart_set_mode(USART3, USART_MODE_TX_RX);  // duplex

    usart_enable(USART3);

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
            usart_send_blocking(USART3, '\r');
        }
        usart_send_blocking(USART3, ptr[i]);
    }

    return i;

}

