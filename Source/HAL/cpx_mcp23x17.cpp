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
    return updateRegisterByTypeAndPin(REG_TYPE::IODIR, port, pinNo, direction);
}

sys_error_t cpx_mcp23x17::setPolarity(PORT port, uint8_t pinNo, bool polarity)
{
    return updateRegisterByTypeAndPin(REG_TYPE::IPOL, port, pinNo, polarity);
}

sys_error_t cpx_mcp23x17::setDefaultValue(PORT port, uint8_t pinNo, bool value)
{
    return updateRegisterByTypeAndPin(REG_TYPE::DEFVAL, port, pinNo, value);
}

sys_error_t cpx_mcp23x17::checkParameters(REG_TYPE regType, PORT port, uint8_t pinNo)
{
    RETURN_IF_ERROR_WITH_LOG(
        (static_cast<uint8_t>(regType) >= REG_TYPE_COUNT) || (static_cast<uint8_t>(port) >= PORT_COUNT) || (pinNo > 7),
        ERROR_INVALID_ARG,    // Error Code
        "Invalid parameters!" // Error Message
    );

    return ERROR_SUCCESS;
}

sys_error_t cpx_mcp23x17::updateRegisterByTypeMasked(REG_TYPE regType, PORT port, const uint8_t mask, const uint8_t value)
{
    if (!_started)
    {
        return ERROR_NOT_INITIALIZED; // Not started, cannot update register
    }

    RETURN_ON_ERROR(checkParameters(regType, port, 0));

    const uint8_t reg = regMap[static_cast<uint8_t const>(regType)][static_cast<uint8_t const>(_bankMode)][static_cast<uint8_t const>(port)];

    return updateRegister(reg, mask, value);
}

sys_error_t cpx_mcp23x17::updateRegisterByTypeAndPin(REG_TYPE regType, PORT port, uint8_t pinNo, const bool value)
{
    uint8_t mask = (1 << pinNo);
    return updateRegisterByTypeMasked(regType, port, mask, value ? mask : 0);
}
