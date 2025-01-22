/**
 * @file Process.hpp
 * @brief Header file for Process
 *
 * This file contains declarations for the Process class and related data types and functions.
 */

#ifndef PROCESS_HPP
#define PROCESS_HPP

#include "System/system.h"
#include "IProcess.hpp"

class Process : public IProcess
{
public:
    enum class State
    {
        INITIALIZED,
        RUNNING,
        PAUSED,
        STOPPED,
    };

    virtual ~Process() {}

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

#endif /* PROCESS_HPP */
