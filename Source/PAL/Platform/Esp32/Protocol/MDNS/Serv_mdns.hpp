/**
 * @file Serv_mdns.hpp
 * @brief Header file for Serv_mdns
 *
 * This file contains declarations for the Serv_mdns class and related data types and functions.
 */

#ifndef SERV_MDNS_HPP
#define SERV_MDNS_HPP

#include "Pal/Pal.h"
#include "System/system.h"
#include <string>

class Serv_mdns : public PAL_Service
{
private:
    std::string _hostname;
    std::string _instanceName;
    uint16_t    _port;

public:
    Serv_mdns(std::string hostname, std::string instanceName, uint16_t port);
    ~Serv_mdns();

    sys_error_t init() override;

    sys_error_t start() override;

    sys_error_t stop() override;

    sys_error_t restart() override;
};
#endif /* SERV_MDNS_HPP */
