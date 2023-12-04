/**
 * @file IProcess.hpp
 * @brief Header file for IProcess
 *
 * This file contains declarations for the IProcess class and related data types and functions.
 */

#ifndef IPROCESS_HPP
#define IPROCESS_HPP

#include "System/system.h"

class IProcess
{
public:
    enum class State
    {
        INITIALIZED,
        RUNNING,
        PAUSED,
        STOPPED,
    };

    virtual ~IProcess() {}

    virtual sys_error_t start()  = 0;
    virtual sys_error_t stop()   = 0;
    virtual sys_error_t pause()  = 0;
    virtual sys_error_t resume() = 0;
    State               getState()
    {
        return _state;
    }
    void setState(State state)
    {
        _state = state;
    }

private:
    State _state;
};

#endif /* IPROCESS_HPP */
