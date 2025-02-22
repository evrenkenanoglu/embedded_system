/**
 * @file Proc_mdns.hpp
 * @brief Header file for Proc_mdns
 *
 * This file contains declarations for the Proc_mdns class and related data types and functions.
 */

#ifndef PROC_MDNS_HPP
#define PROC_MDNS_HPP

#include "PAL/PAL.h"
#include "Process/Process.hpp"

class Proc_mdns : public Process
{

public:
    Proc_mdns(IPAL_Service& networkService);
    ~Proc_mdns();

    sys_error_t start() override;

    sys_error_t stop() override;

    sys_error_t pause() override;

    sys_error_t resume() override;

private:
    IPAL_Service& _networkService;
};
#endif /* PROC_MDNS_HPP */
