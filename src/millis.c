#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/systick.h>

#include "millis.h"

extern void sys_tick_handler(void);

static volatile uint64_t _millis = 0;

void millis_setup(void) {

    // Set the systick clock source to our main clock
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);

    // Clear the Current Value Register so that we start at 0
    STK_CVR = 0;

    // In order to trigger an interrupt every millisecond, we can set the reload
    // value to be the speed of the processor / 1000 - 1
    systick_set_reload(rcc_ahb_frequency / 1000 - 1);

    // Enable interrupts from the system tick clock
    systick_interrupt_enable();

    // Enable the system tick counter
    systick_counter_enable();

}

void sys_tick_handler(void) {

    _millis++;

}

uint64_t millis(void) {

    return _millis;

}

