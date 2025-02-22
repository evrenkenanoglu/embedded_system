/**
 * @file Proc_mdns.cpp
 * @brief Source file for Proc_mdns
 *
 * This file contains definitions for the Proc_mdns class and related data types and functions.
 */

#include "Proc_mdns.hpp"

Proc_mdns::Proc_mdns(IPAL_Service& networkService) : _networkService(networkService)
{
    setState(Process::State::INITIALIZED);
}

Proc_mdns::~Proc_mdns()
{
    // destructor implementation
}

sys_error_t Proc_mdns::start()
{
    RETURN_ON_ERROR(_networkService.init());
    RETURN_ON_ERROR(_networkService.start());
    setState(Process::State::RUNNING);
    return ERROR_SUCCESS;
}

sys_error_t Proc_mdns::stop()
{
    RETURN_ON_ERROR(_networkService.stop());
    setState(Process::State::STOPPED);
    return ERROR_SUCCESS;
}

sys_error_t Proc_mdns::pause()
{
    return ERROR_NOT_IMPLEMENTED;
}

sys_error_t Proc_mdns::resume()
{
    return ERROR_NOT_IMPLEMENTED;
}