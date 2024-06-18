/**
 * @file Proc_httpServer.hpp
 * @brief Header file for Proc_httpServer
 *
 * This file contains declarations for the Proc_httpServer class and related data types and functions.
 */

#ifndef PROC_HTTPSERVER_HPP
#define PROC_HTTPSERVER_HPP

#include "HAL/Platform/ESP32/cpx_wifi.h"
#include "Process.hpp"
#include <esp_http_server.h>

typedef struct
{
    IProcess* process;
    void*     htmlContext;
} httpUriContext_t;

class Proc_httpServer : public Process
{
private:
    httpd_handle_t      _server;
    httpd_config_t      _config;
    EventGroupHandle_t& _wifiConfigEventGroup;
    QueueHandle_t       _wifiConfigScanResults;
    const char*         _welcomeWifiConnectHtml;

    const httpd_uri_t welcome;
    const httpd_uri_t scan;
    const httpd_uri_t connect;

public:
    Proc_httpServer(EventGroupHandle_t& wifiConfigEventGroup, QueueHandle_t wifiConfigScanResults);
    ~Proc_httpServer();

    sys_error_t start() override;

    sys_error_t stop() override;

    sys_error_t pause() override;

    sys_error_t resume() override;

public:
    /**
     * @brief Get the welcome wifi connect html content
     *
     * @return const char*
     */
    const char* getWelcomeWifiConnectHtml() const;

    /**
     * @brief Get the Wifi Config Event Group Handle
     *
     * @return EventGroupHandle_t&
     */
    EventGroupHandle_t& getWifiConfigEventGroup();

    QueueHandle_t getWifiConfigScanResults();
};

#endif /* PROC_HTTPSERVER_HPP */
