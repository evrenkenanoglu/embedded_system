/**
 * @file Serv_mdns.cpp
 * @brief Source file for Serv_mdns
 *
 * This file contains definitions for the Serv_mdns class and related data types and functions.
 */

#include "Serv_mdns.hpp"
#include "System/LogHandler.h"
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
    error_t error = mdns_init();
    if (error != ESP_OK)
    {
        SYS_LOG_E("Failed to initialize mDNS");
        return ERROR_FAIL;
    }

    SYS_LOG_I( "mDNS initialized");

    return ERROR_SUCCESS;
}

sys_error_t Serv_mdns::start()
{
    // Initialize mDNS
    error_t error = mdns_hostname_set(_hostname.c_str());
    if (error != ESP_OK)
    {
        SYS_LOG_E("Failed to set mDNS hostname");
        return ERROR_FAIL;
    }

    error = mdns_instance_name_set(_instanceName.c_str());

    if (error != ESP_OK)
    {
        SYS_LOG_E("Failed to set mDNS instance name");
        return ERROR_FAIL;
    }

    // Set mDNS service
    error = mdns_service_add("ESP32-WebServer", "_https", "_tcp", _port, NULL, 0);

    if (error != ESP_OK)
    {
        SYS_LOG_E("Failed to add mDNS service");
        return ERROR_FAIL;
    }

    error = mdns_service_txt_item_set("_https", "_tcp", "path", "/");

    if (error != ESP_OK)
    {
        SYS_LOG_E("Failed to set mDNS service text item");
        return ERROR_FAIL;
    }

    SYS_LOG_I( "mDNS service started!");
    return ERROR_SUCCESS;
}

sys_error_t Serv_mdns::stop()
{
    mdns_free();
    SYS_LOG_I( "mDNS stopped!");
    return ERROR_SUCCESS;
}

sys_error_t Serv_mdns::restart()
{
    RETURN_ON_ERROR(stop());
    RETURN_ON_ERROR(init());
    RETURN_ON_ERROR(start());
    return ERROR_SUCCESS;
}