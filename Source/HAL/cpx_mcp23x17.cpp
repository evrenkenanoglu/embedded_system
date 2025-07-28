/**
 * @file cpx_mcp23x17.cpp
 * @brief Source file for cpx_mcp23x17
 *
 * This file contains definitions for the cpx_mcp23x17 class and related data types and functions.
 */

#include "cpx_mcp23x17.hpp"
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"

cpx_mcp23x17::cpx_mcp23x17(IHAL_COM& comInterface, REG_BANK_MODE bankMode)
    : _started(false)
    , _comInterface(comInterface)
    , _bankMode(bankMode)
{
    // constructor implementation
}

cpx_mcp23x17::~cpx_mcp23x17()
{
    // destructor implementation
}

sys_error_t cpx_mcp23x17::start()
{

    if (_started)
    {
        return ERROR_SUCCESS; // Already started
    }

    return ERROR_SUCCESS;
}

void* cpx_mcp23x17::get()
{
    if (!_started)
    {
        return nullptr; // Not started, return null
    }

    return nullptr; // Placeholder, replace with actual data retrieval logic
}

sys_error_t cpx_mcp23x17::set(void* data)
{
    if (!_started)
    {
        return ERROR_NOT_INITIALIZED; // Not started, cannot set data
    }

    // Set the data for the complex operation (implementation depends on the specific use case)
    // Placeholder, replace with actual data setting logic

    return ERROR_SUCCESS;
}

sys_error_t cpx_mcp23x17::stop()
{
    if (!_started)
    {
        return ERROR_NOT_INITIALIZED; // Not started, cannot stop
    }

    _started = false; // Mark as stopped

    // Additional cleanup logic if necessary

    return ERROR_SUCCESS;
}

sys_error_t cpx_mcp23x17::init() {}

sys_error_t cpx_mcp23x17::writeRegister(const uint8_t reg, const uint8_t value)
{
    if (!_started)
    {
        return ERROR_NOT_INITIALIZED; // Not started, cannot write register
    }
    // Prepare the data to be sent
    uint8_t data[3];
    data[0] = static_cast<uint8_t>(MCP23017_I2C_ADDRESS); // MCP23017 I2C address
    data[1] = static_cast<uint8_t>(reg);                  // Register address
    data[2] = value;                                      // Value to write

    // Send the data using the communication interface
    RETURN_ON_ERROR_WITH_LOG(
        _comInterface.sendData(data, sizeof(data)), // Function Call
        "Failed to write register",                 // Error Message
    );

    do
    {
        sys_error_t err = static_cast<sys_error_t>(_comInterface.sendData(data, sizeof(data)));
        if (err != ERROR_SUCCESS)
        {
            LogHandler::getInstance().log(
                ILog::LogLevel::ERROR,
                "Error in %s at line %d: Error Code: %d - %s",
                "C:\\Workspace_Personal\\ESP32\\Embedded_IoT_BT_WIFI_Base_Project\\embedded_system\\Source\\HAL\\cpx_mcp23x17.cpp",
                91,
                err,
                "Failed to write register");
            ;
            return err;
        }
    } while (0);

    return ERROR_SUCCESS;
}

sys_error_t cpx_mcp23x17::readRegister(const uint8_t reg, uint8_t& value)
{
    if (!_started)
    {
        return ERROR_NOT_INITIALIZED; // Not started, cannot read register
    }

    // Prepare the data to be sent
    uint8_t writeBuffer[2];
    writeBuffer[0] = static_cast<uint8_t>(MCP23017_I2C_ADDRESS); // MCP23017 I2C address
    writeBuffer[1] = static_cast<uint8_t>(reg);                  // Register address

    // Read the value from the register using the communication interface
    RETURN_ON_ERROR_WITH_LOG(
        _comInterface.writeRead(writeBuffer, sizeof(writeBuffer), &value, sizeof(value)), // Function Call
        "Failed to read register",                                                        // Error Message
    );

    return ERROR_SUCCESS;
}

sys_error_t cpx_mcp23x17::updateRegister(const uint8_t reg, uint8_t mask, uint8_t value)
{
    if (!_started)
    {
        return ERROR_NOT_INITIALIZED; // Not started, cannot update register
    }

    uint8_t currentValue;
    RETURN_ON_ERROR_WITH_LOG(
        readRegister(reg, currentValue),         // Function Call
        "Failed to read current register value", // Error Message
    );

    // Update the value with the mask
    currentValue = (currentValue & ~mask) | (value & mask);

    // Write the updated value back to the register
    RETURN_ON_ERROR_WITH_LOG(
        writeRegister(reg, currentValue),         // Function Call
        "Failed to write updated register value", // Error Message
    );

    return ERROR_SUCCESS;
}

sys_error_t cpx_mcp23x17::setIODirection(PORT port, uint8_t pinNo, bool direction)
{
    if (!_started)
    {
        return ERROR_NOT_INITIALIZED; // Not started, cannot set I/O direction
    }

    uint8_t reg = regMap[REG_TYPE::IODIR][_bankMode][port];

    uint8_t mask = (1 << pinNo);

    // Update the I/O direction register
    sys_error_t err = updateRegister(static_cast<uint8_t>(reg), mask, direction ? mask : 0);
    if (err != ERROR_SUCCESS)
    {
        return err; // Return the error code if the operation failed
    }

    return ERROR_SUCCESS;
}

sys_error_t cpx_mcp23x17::enableIOInterrupt(PORT port, uint8_t pinNo, bool enable)
{
    if (!_started)
    {
        return ERROR_NOT_INITIALIZED; // Not started, cannot enable I/O interrupt
    }

    uint8_t reg = regMap[REG_TYPE::]

        uint8_t mask = (1 << pinNo);

    // Update the GPIO interrupt enable register
    sys_error_t err = updateRegister(static_cast<uint8_t>(reg), mask, enable ? mask : 0);
    if (err != ERROR_SUCCESS)
    {
        return err; // Return the error code if the operation failed
    }

    return ERROR_SUCCESS;
}

sys_error_t cpx_mcp23x17::setPolarity(PORT port, uint8_t pinNo, bool polarity)
{
    if (!_started)
    {
        return ERROR_NOT_INITIALIZED; // Not started, cannot set polarity
    }

    // Determine the register and bit to set based on port and pin number
    REG_BANK0 reg  = (port == PORT::A) ? REG_BANK0::IPOLA : REG_BANK0::IPOLB;
    uint8_t   mask = (1 << pinNo);

    // Update the input polarity register
    sys_error_t err = updateRegister(static_cast<uint8_t>(reg), mask, polarity ? mask : 0);
    if (err != ERROR_SUCCESS)
    {
        return err; // Return the error code if the operation failed
    }

    return ERROR_SUCCESS;
}
