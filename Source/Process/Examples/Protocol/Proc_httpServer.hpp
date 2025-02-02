/**
 * @file Proc_httpServer.hpp
 * @brief Header file for Proc_httpServer
 *
 * This file contains declarations for the Proc_httpServer class and related data types and functions.
 */

#ifndef PROC_HTTPSERVER_HPP
#define PROC_HTTPSERVER_HPP

#include "PAL/PAL.h"
#include "Process/Process.hpp"

class Proc_httpServer : public Process
{
public:
    Proc_httpServer(IPAL_NetworkService& networkService);
    ~Proc_httpServer();

    sys_error_t start() override;

    sys_error_t stop() override;

    sys_error_t pause() override;

    sys_error_t resume() override;

private:
    IPAL_NetworkService& _networkService;
};

#endif /* PROC_HTTPSERVER_HPP */
