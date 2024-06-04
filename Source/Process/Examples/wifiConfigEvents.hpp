#ifndef WIFI_CONFIG_EVENTS_HPP
#define WIFI_CONFIG_EVENTS_HPP

#include "esp_bit_defs.h"

// Define event bits
#define WIFI_CONFIG_AP_SETUP_READY          BIT0 // Access Point Setup Ready
#define WIFI_CONFIG_AP_SETUP_FINISH         BIT1 // Access Point Setup Shutdown

#define WIFI_CONFIG_STA_SETUP_READY         BIT2 // Station Setup Ready
#define WIFI_CONFIG_STA_SETUP_FINISH        BIT3 // Station Setup Shutdown

#define WIFI_CONFIG_SCAN_REQUESTED          BIT4
#define WIFI_CONFIG_SCAN_DONE               BIT5

#define WIFI_CONFIG_CONNECTED_TO_AP         BIT6
#define WIFI_CONFIG_DISCONNECTED_FROM_AP    BIT7

#define WIFI_CONFIG_TRY_CONNECT             BIT2
#define WIFI_CONFIG_CONNECTED               BIT3
#define WIFI_CONFIG_DISCONNECTED            BIT4
#define WIFI_CONFIG_CONNECTION_FAILED       BIT5
#define WIFI_CONFIG_WAITING_FOR_CREDENTIALS BIT0
#define WIFI_CONFIG_CREDENTIALS_STORED_BIT  BIT6

#endif // WIFI_CONFIG_EVENTS_HPP