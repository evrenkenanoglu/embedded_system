/**
 * @file Serv_mdns.cpp
 * @brief Source file for Serv_mdns
 *
 * This file contains definitions for the Serv_mdns class and related data types and functions.
 */

#include "Serv_mdns.hpp"
#include "mdns.h"

Serv_mdns::Serv_mdns(std::string hostname, std::string instanceName, uint16_t port)
    : _hostname(hostname)
    , _instanceName(instanceName)
    , _port(port)
{
    // constructor implementation
}

Serv_mdns::~Serv_mdns()
{
    // destructor implementation
}

sys_error_t Serv_mdns::init()
{
    ESP_ERROR_CHECK(mdns_init());
    return ERROR_SUCCESS;
}

sys_error_t Serv_mdns::start()
{
    // Initialize mDNS
    ESP_ERROR_CHECK(mdns_hostname_set(_hostname.c_str()));
    ESP_ERROR_CHECK(mdns_instance_name_set(_instanceName.c_str()));

    // Set mDNS service
    ESP_ERROR_CHECK(mdns_service_add("ESP32-WebServer", "_http", "_tcp", _port, NULL, 0));
    ESP_ERROR_CHECK(mdns_service_txt_item_set("_http", "_tcp", "path", "/"));

    return ERROR_SUCCESS;
}

sys_error_t Serv_mdns::stop()
{
    mdns_free();
    return ERROR_SUCCESS;
}

sys_error_t Serv_mdns::restart()
{
    RETURN_ON_ERROR(stop());
    RETURN_ON_ERROR(init());
    RETURN_ON_ERROR(start());
    return ERROR_SUCCESS;
}