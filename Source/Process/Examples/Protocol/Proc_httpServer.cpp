/**
 * @file Proc_httpServer.cpp
 * @brief Source file for Proc_httpServer
 *
 * This file contains definitions for the Proc_httpServer class and related data types and functions.
 */

#include "Proc_httpServer.hpp"
#include "HAL/Platform/ESP32/library/logImpl.h"

Proc_httpServer::Proc_httpServer(IPAL_Service& networkService)
    : _networkService(networkService)
{
    setState(Process::State::INITIALIZED);
}

Proc_httpServer::~Proc_httpServer()
{
    // destructor implementation
}

sys_error_t Proc_httpServer::start()
{
    RETURN_ON_ERROR(_networkService.init());
    RETURN_ON_ERROR(_networkService.start());
    setState(Process::State::RUNNING);
    return ERROR_SUCCESS;
}

sys_error_t Proc_httpServer::stop()
{
    RETURN_ON_ERROR(_networkService.stop());
    setState(Process::State::STOPPED);
    return ERROR_SUCCESS;
}

sys_error_t Proc_httpServer::pause()
{
    return ERROR_NOT_IMPLEMENTED;
}

sys_error_t Proc_httpServer::resume()
{
    return ERROR_NOT_IMPLEMENTED;
}