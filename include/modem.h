#ifndef MODEM_H
#define MODEM_H

#define MODEM_CTO_MS 100 // character timeout in ms
#define MODEM_BUF_SIZE 512

void modem_setup(void);
bool modem_init(void);
bool modem_wait_until_ready(uint64_t);

void modem_power_up(void);
void modem_power_down(void);
void modem_reset(void);

bool modem_get_imei(void);
char *modem_imei_str(void);

void modem_get_rssi(void);

#endif
