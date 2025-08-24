/**
 * @file cpx_mcp23x17.cpp
 * @brief Source file for cpx_mcp23x17
 *
 * This file contains definitions for the cpx_mcp23x17 class and related data types and functions.
 */

#include "cpx_mcp23x17.hpp"
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
// MACRO DEFINITIONS
/////////////////////////////////////////////////////////////////////////////////////////////////

#define CPX_MCP23X17_REG_FUNCS(NAME, REGTYPE)                                                \
    sys_error_t cpx_mcp23x17::set##NAME##No(PORT port, uint8_t pinNo, bool value)            \
    {                                                                                        \
        return updateRegisterByTypePortBit(REG_TYPE::REGTYPE, port, pinNo, value, true);     \
    }                                                                                        \
    sys_error_t cpx_mcp23x17::get##NAME##No(PORT port, uint8_t pinNo, bool& value)           \
    {                                                                                        \
        return readRegisterByTypeBit(REG_TYPE::REGTYPE, port, pinNo, value);                 \
    }                                                                                        \
    sys_error_t cpx_mcp23x17::set##NAME(PORT port, uint8_t value)                            \
    {                                                                                        \
        return updateRegisterByTypePortMaskByte(REG_TYPE::REGTYPE, port, 0xFF, value, true); \
    }                                                                                        \
    sys_error_t cpx_mcp23x17::get##NAME(PORT port, uint8_t& value)                           \
    {                                                                                        \
        return readRegisterByTypeByte(REG_TYPE::REGTYPE, port, value);                       \
    }

cpx_mcp23x17::cpx_mcp23x17(IHAL_COM& comInterface, REG_BANK_MODE bankMode, IHAL_IO* interruptPinA, IHAL_IO* interruptPinB)
    : _isInitialized(false)                                                           // Initialize as not initialized
    , _started(false)                                                                 // Initialize started state
    , _comInterface(comInterface)                                                     // Initialize communication interface
    , _interruptPinA(interruptPinA)                                                   // Initialize interrupt pin A
    , _interruptPinB(interruptPinB)                                                   // Initialize interrupt pin B
    , _deviceAddress(MCP23017_I2C_ADDRESS)                                            // Default I2C address for MCP23017
    , _bankMode(bankMode)                                                             // 7th bit
    , _isMirrorEnabled(static_cast<bool>(MIRROR_DISABLED))                            // 6th bit
    , _isSequentialOperationDisabled(static_cast<bool>(SEQENTIAL_OPERATION_DISABLED)) // 5th bit
    , _isSlewRateDisabled(static_cast<bool>(SLEW_RATE_ENABLED))                       // 4th bit
    , _isHardwareAddressEnabled(static_cast<bool>(HAEN_ENABLED))                      // 3rd bit
    , _isOpenDrainEnabled(static_cast<bool>(ODR_DISABLED))                            // 2nd bit
    , _isIntPolarityActiveHigh(static_cast<bool>(INTPOL_ACTIVE_LOW))                  // 1st bit ,
// 0th bit no effect
{
    // constructor implementation
}

cpx_mcp23x17::~cpx_mcp23x17()
{
    // destructor implementation
}

sys_error_t cpx_mcp23x17::init(void* params)
{

    if (_isInitialized)
    {
        return ERROR_SUCCESS; // Already initialized
    }

    uint8_t ioconValueA = 0;
    uint8_t ioconValueB = 0;
    SYS_LOG_I("IOCON Register A: 0x%02X, B: 0x%02X", ioconValueA, ioconValueB);
    setIOCONRegister();
    readRegisterByTypeByte(REG_TYPE::IOCON, PORT::A, ioconValueA);
    readRegisterByTypeByte(REG_TYPE::IOCON, PORT::B, ioconValueB);

    SYS_LOG_I("IOCON Register A: 0x%02X, B: 0x%02X", ioconValueA, ioconValueB);

    _isInitialized = true; // Mark as initialized

    return ERROR_SUCCESS;
}

sys_error_t cpx_mcp23x17::deInit()
{
    if (!_isInitialized)
    {
        return ERROR_NOT_INITIALIZED; // Not initialized, cannot deinitialize
    }

    _isInitialized = false; // Mark as not initialized

    // Additional cleanup logic if necessary

    return ERROR_SUCCESS;
}

sys_error_t cpx_mcp23x17::start()
{

    if (!_isInitialized)
    {
        return ERROR_NOT_INITIALIZED; // Already initialized
    }

    if (_started)
    {
        return ERROR_SUCCESS; // Already started
    }
    _started = true; // Mark as started

    // Initialize interrupt pins if provided
    if (_interruptPinA)
    {
        RETURN_ON_ERROR_WITH_LOG(
            _interruptPinA->set(nullptr),   // Set the interrupt pin A
            "Failed to set interrupt pin A" // Error Message
        );
    }

    // create a task to handle interrupts if needed

    return ERROR_SUCCESS;
}

sys_error_t cpx_mcp23x17::get(void* data)
{
    if (data == nullptr)
    {
        return ERROR_INVALID_ARG; // Invalid argument
    }

    return ERROR_NOT_IMPLEMENTED; // Placeholder, replace with actual data retrieval logic
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

sys_error_t cpx_mcp23x17::init()
{

    return ERROR_SUCCESS;
}

sys_error_t cpx_mcp23x17::setBankMode(REG_BANK_MODE bankMode)
{
    if (bankMode != _bankMode)
    {
        _bankMode = bankMode;
        return setIOCONRegister();
    }
    return ERROR_SUCCESS; // No change needed
}

sys_error_t cpx_mcp23x17::setIOCONRegister()
{
    // Set IOCON register bits based on the current configuration
    uint8_t ioconBits = (static_cast<bool>(_bankMode) ? static_cast<uint8_t>(IOCON_BITS::BANK) : 0) |    // bank mode
                        (_isMirrorEnabled ? static_cast<uint8_t>(IOCON_BITS::MIRROR) : 0) |              // interrupt mirror
                        (_isSequentialOperationDisabled ? static_cast<uint8_t>(IOCON_BITS::SEQOP) : 0) | // sequential operation
                        (_isSlewRateDisabled ? static_cast<uint8_t>(IOCON_BITS::DISSLW) : 0) |           // slew rate control
                        (_isHardwareAddressEnabled ? static_cast<uint8_t>(IOCON_BITS::HAEN) : 0) |       // hardware address enable
                        (_isOpenDrainEnabled ? static_cast<uint8_t>(IOCON_BITS::ODR) : 0) |              // open-drain mode
                        (_isIntPolarityActiveHigh ? static_cast<uint8_t>(IOCON_BITS::INTPOL) : 0)        // interrupt polarity
        ;

    SYS_LOG_D("Setting IOCON register with bits: 0x%02X", ioconBits);
    // Update the IOCON register with the provided bits

    return updateRegisterByTypePortMaskByte(REG_TYPE::IOCON, PORT::A, 0xFF, ioconBits, true);
}

CPX_MCP23X17_REG_FUNCS(Direction, IODIR);
CPX_MCP23X17_REG_FUNCS(Polarity, IPOL);
CPX_MCP23X17_REG_FUNCS(InterruptEnable, GPINTEN);
CPX_MCP23X17_REG_FUNCS(DefaultValue, DEFVAL);
CPX_MCP23X17_REG_FUNCS(InterruptControl, INTCON);
CPX_MCP23X17_REG_FUNCS(PullUpResistor, GPPU);
CPX_MCP23X17_REG_FUNCS(InterruptFlag, INTF);
CPX_MCP23X17_REG_FUNCS(InterruptCaptured, INTCAP);
CPX_MCP23X17_REG_FUNCS(Gpio, GPIO);
CPX_MCP23X17_REG_FUNCS(Latch, OLAT);

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// REGISTER OPERATIONS
/////////////////////////////////////////////////////////////////////////////////////////////////////////

sys_error_t cpx_mcp23x17::writeRegister(const uint8_t reg, const uint8_t value)
{
    if (!_started)
    {
        return ERROR_NOT_INITIALIZED; // Not started, cannot write register
    }

    // Prepare the data to be sent
    const uint8_t data[2] = {static_cast<uint8_t>(reg), value}; // Register address and value to write
    SYS_LOG_D("Writing to register: 0x%02X, value: 0x%02X", reg, value);

    // Send the data using the communication interface
    RETURN_ON_ERROR_WITH_LOG(
        _comInterface.sendData(&_deviceAddress, data, sizeof(data)), // Function Call
        "Failed to write register",                                  // Error Message
    );

    return ERROR_SUCCESS;
}

sys_error_t cpx_mcp23x17::readRegister(const uint8_t reg, uint8_t& value)
{
    if (!_started)
    {
        return ERROR_NOT_INITIALIZED; // Not started, cannot read register
    }

    // Read the value from the register using the communication interface
    RETURN_ON_ERROR_WITH_LOG(
        _comInterface.writeRead(&_deviceAddress, &reg, sizeof(reg), &value, sizeof(value)), // Function Call
        "Failed to read register",                                                          // Error Message
    );
    SYS_LOG_D("Read from register: 0x%02X, value: 0x%02X", reg, value);
    return ERROR_SUCCESS;
}

sys_error_t cpx_mcp23x17::readRegisterByTypeByte(const REG_TYPE regType, const PORT port, uint8_t& value)
{
    RETURN_ON_ERROR(checkParameters(regType, port, 0));

    const uint8_t reg = regMap[static_cast<uint8_t const>(regType)][static_cast<uint8_t const>(_bankMode)][static_cast<uint8_t const>(port)];

    return readRegister(reg, value);
}

sys_error_t cpx_mcp23x17::readRegisterByTypeBit(const REG_TYPE regType, const PORT port, const uint8_t bitNo, bool& value)
{
    RETURN_ON_ERROR(checkParameters(regType, port, bitNo));

    uint8_t regValue;
    RETURN_ON_ERROR_WITH_LOG(
        readRegisterByTypeByte(regType, port, regValue), // Function Call
        "Failed to read register value",                 // Error Message
    );

    value = (regValue & (1 << bitNo)) != 0; // Check if the specific bit is set
    return ERROR_SUCCESS;
}

sys_error_t cpx_mcp23x17::updateRegisterMasked(const uint8_t reg, const uint8_t mask, const uint8_t value, const bool verify)
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
    SYS_LOG_D("Updating register: 0x%02X, current value: 0x%02X, mask: 0x%02X, new value: 0x%02X", reg, currentValue, mask, value);
    // Write the updated value back to the register
    RETURN_ON_ERROR_WITH_LOG(
        writeRegister(reg, currentValue),         // Function Call
        "Failed to write updated register value", // Error Message
    );
    SYS_LOG_D("Register 0x%02X updated successfully", reg);

    if (verify)
    {
        SYS_LOG_D("Verifying register: 0x%02X after update", reg);
        // Verify the register value if required
        RETURN_ON_ERROR_WITH_LOG(
            verifyRegister(reg, currentValue), // Function Call
            "Failed to verify register value", // Error Message
        );
    }

    return ERROR_SUCCESS;
}

sys_error_t cpx_mcp23x17::verifyRegister(const uint8_t reg, const uint8_t expectedValue)
{
    if (!_started)
    {
        return ERROR_NOT_INITIALIZED; // Not started, cannot verify register
    }

    SYS_LOG_D("Verifying register: 0x%02X, expected value: 0x%02X", reg, expectedValue);
    uint8_t currentValue;
    RETURN_ON_ERROR_WITH_LOG(
        readRegister(reg, currentValue),         // Function Call
        "Failed to read current register value", // Error Message
    );
    SYS_LOG_D("Verifying register: 0x%02X, expected value: 0x%02X, current value: 0x%02X", reg, expectedValue, currentValue);
    if (currentValue != expectedValue)
    {
        return ERROR_READ_FAILED; // Verification failed, current value does not match expected value
    }

    return ERROR_SUCCESS;
}

sys_error_t cpx_mcp23x17::updateRegisterByTypePortMaskByte(const REG_TYPE regType, const PORT port, const uint8_t mask, const uint8_t value, const bool verify)
{
    RETURN_ON_ERROR(checkParameters(regType, port, 0));

    const uint8_t reg = regMap[static_cast<uint8_t const>(regType)][static_cast<uint8_t const>(_bankMode)][static_cast<uint8_t const>(port)];

    return updateRegisterMasked(reg, mask, value, verify);
}

sys_error_t cpx_mcp23x17::updateRegisterByTypePortBit(const REG_TYPE regType, const PORT port, const uint8_t bitNo, const bool value, const bool verify)
{
    uint8_t mask = (1 << bitNo);
    return updateRegisterByTypePortMaskByte(regType, port, mask, value ? mask : 0, verify);
}

sys_error_t cpx_mcp23x17::checkParameters(const REG_TYPE regType, const PORT port, const uint8_t bitNo)
{
    RETURN_IF_ERROR_WITH_LOG(
        (static_cast<uint8_t>(regType) >= REG_TYPE_COUNT) || (static_cast<uint8_t>(port) >= PORT_COUNT) || (bitNo > 7),
        ERROR_INVALID_ARG,    // Error Code
        "Invalid parameters!" // Error Message
    );

    return ERROR_SUCCESS;
}
