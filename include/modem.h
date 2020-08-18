#ifndef MODEM_H
#define MODEM_H

#define MODEM_CTO_MS 100 // character timeout in ms
#define MODEM_BUF_SIZE 512

bool modem_TEST(void);

void modem_setup(void);
bool modem_init(void);
bool modem_wait_until_ready(uint64_t);

void modem_power_up(void);
void modem_power_down(void);
void modem_reset(void);

bool modem_get_network_registration(uint8_t*);
bool modem_get_network_system_mode(uint8_t*);
bool modem_get_functionality(uint8_t*);
bool modem_get_available_networks(void);
bool modem_get_imsi(void);
bool modem_get_imei(void);
bool modem_get_firmware_version(void);
char *modem_imei_str(void);

uint8_t *modem_get_buffer_data(void);
char *modem_get_buffer_string(void);

bool modem_connect_bearer(void);
bool modem_post_temperature(float);
bool modem_query_bearer(void);

bool modem_get_rssi_ber(uint8_t*, uint8_t*);

bool modem_gps_enable(void);
bool modem_gps_get_nav(void);


#endif
