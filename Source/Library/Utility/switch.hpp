#pragma once

#include "HAL/IHAL/IHal_Io_Gpio.h"
#include "System/system.h"

namespace SWITCH
{

enum class Event
{
    STATE_CHANGED,
    MAX
};

enum class State
{
    OFF,
    ON,
    TOGGLE,
    UNKNOWN
};

typedef struct
{
    uint16_t index; // Unique index for the switch instance
    Event    event;
    State    state;
} EventData_t;

typedef struct
{
    uint16_t      index;      // Unique index for the switch instance
    IHAL_IO_GPIO& gpio;       // Pointer to the GPIO interface controlling the switch
    bool          isInverted; // true if HIGH means OFF and LOW means ON
    State         state;      // Current state of the switch
} Instance_t;

constexpr Instance_t CREATE_INSTANCE(uint8_t index, IHAL_IO_GPIO& gpio, bool isInverted = false)
{
    return Instance_t{.index = index, .gpio = gpio, .isInverted = isInverted, .state = State::OFF};
}

} // namespace SWITCH
