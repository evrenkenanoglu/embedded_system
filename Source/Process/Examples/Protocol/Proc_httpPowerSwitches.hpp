/**
 * @file Proc_httpPowerSwitches.hpp
 * @brief Header file for Proc_httpPowerSwitches
 *
 * This file contains declarations for the Proc_httpPowerSwitches class and related data types and functions.
 */

#ifndef PROC_HTTPPOWERSWITCHES_HPP
#define PROC_HTTPPOWERSWITCHES_HPP

#include "Process/Examples/Peripheral/Proc_Switches.hpp"
#include "Process/Process.hpp"
#include <esp_http_server.h>

class Proc_httpPowerSwitches : public Process
{
private:
    QueueHandle_t  _switchesQueue;

    httpd_handle_t _server;
    httpd_config_t _config;
    const char*    _powerSwitchesHtml;

    const httpd_uri_t appInterface;
    const httpd_uri_t control;

public:
    Proc_httpPowerSwitches(QueueHandle_t switchesQueue);
    ~Proc_httpPowerSwitches();

    sys_error_t start() override;

    sys_error_t stop() override;

    sys_error_t pause() override;

    sys_error_t resume() override;

    const char* getPowerSwitchesHtml() const;

    QueueHandle_t getSwitchesQueue();
};

#endif /* PROC_HTTPPOWERSWITCHES_HPP */
