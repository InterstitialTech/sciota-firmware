#ifndef MODEM_H
#define MODEM_H

void modem_setup(void);
void modem_init(void);

void modem_power_up(void);
void modem_power_down(void);
void modem_reset(void);

void modem_get_imei(void);
void modem_get_rssi(void);

#endif
