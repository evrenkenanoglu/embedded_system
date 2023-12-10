#ifndef ESP_WIFI_TYPES_H
#define ESP_WIFI_TYPES_H

#include <stdint.h>

typedef enum {
    WIFI_MODE_NULL,
    WIFI_MODE_STA,
    WIFI_MODE_AP,
    WIFI_MODE_APSTA,
    WIFI_MODE_MAX,
} wifi_mode_t;

typedef struct {
    uint8_t mac[6];
} wifi_sta_info_t;

typedef struct {
    char ssid[32];
    char password[64];
    uint8_t ssid_len;
    uint8_t channel;
    uint8_t authmode;
    bool ssid_hidden;
    uint8_t max_connection;
} wifi_config_t;

typedef struct {
    wifi_mode_t mode;
    wifi_config_t ap;
    wifi_config_t sta;
    uint8_t country[3];
} wifi_init_config_t;

#endif // ESP_WIFI_TYPES_H