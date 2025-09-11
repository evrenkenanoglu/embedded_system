/**
 * @file com_i2c.cpp
 * @brief Source file for com_i2c
 *
 * This file contains definitions for the com_i2c class and related data types
 * and functions.
 */

#include "com_i2c.hpp"
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"

#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */

#define I2C_MASTER_TIMEOUT_MS     1000
#define COM_TIMEOUT_MS            I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS

#define I2C_ADDRESS_SIZE          1                                      /*!< I2C address size in bytes */
#define I2C_REGISTER_SIZE         1                                      /*!< I2C register size in bytes */
#define I2C_WRITE_SIZE            (I2C_ADDRESS_SIZE + I2C_REGISTER_SIZE) /*!< I2C write size in bytes */

com_i2c::com_i2c(i2c_port_t i2cPort, i2c_config_t& config)
    : _config(config)
    , _i2cPort(i2cPort)
    , _i2cMutex(nullptr)
    , _isInitialized(false)
{
    // constructor implementation
}

com_i2c::~com_i2c()
{
    // destructor implementation
}

sys_error_t com_i2c::init(void* params)
{
    UNUSED(params);

    if (_isInitialized)
    {
        return ERROR_SUCCESS; // Already initialized
    }

    if (_i2cMutex != nullptr)
    {
        vSemaphoreDelete(_i2cMutex); // Delete the existing mutex if it exists
        _i2cMutex = nullptr;
    }

    // Create a mutex for I2C operations
    _i2cMutex = xSemaphoreCreateMutex();

    xSemaphoreTake(_i2cMutex, portMAX_DELAY); // Take the mutex to ensure exclusive access

    SYS_LOG_D("Initializing I2C on port %d with clock speed %d", _i2cPort, _config.master.clk_speed);
    // Check if the mutex was created successfully
    RETURN_IF_ERROR_WITH_LOG((_i2cMutex == nullptr), ERROR_MEMORY, "Failed to create I2C mutex", xSemaphoreGive(_i2cMutex));

    SYS_LOG_D("I2C mutex created successfully");
    // Initialize I2C with the provided configuration
    RETURN_ON_ERROR_WITH_LOG(i2c_param_config(_i2cPort, &_config), "Failed to configure I2C parameters", xSemaphoreGive(_i2cMutex));

    SYS_LOG_D("I2C parameters configured successfully");
    // Install the I2C driver
    RETURN_ON_ERROR_WITH_LOG(
        i2c_driver_install(_i2cPort, _config.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0), "Failed to install I2C driver", xSemaphoreGive(_i2cMutex));

    xSemaphoreGive(_i2cMutex); // Release the mutex after initialization

    SYS_LOG_D("I2C initialized on port %d with clock speed %d", _i2cPort, _config.master.clk_speed);

    _isInitialized = true; // Mark the I2C as initialized

    return ERROR_SUCCESS;
}

sys_error_t com_i2c::connect()
{
    return ERROR_SUCCESS;
}

sys_error_t com_i2c::sendData(const void* deviceAddress, const uint8_t* data, size_t length)
{

    xSemaphoreTake(_i2cMutex, portMAX_DELAY); // Take the semaphore to ensure exclusive access to I2C operations

    // Validate input arguments
    RETURN_IF_ERROR_WITH_LOG(
        (deviceAddress == nullptr),      // Expression
        ERROR_INVALID_ARG,               // Error Code
        "Invalid data buffer or length", // Error Message
        xSemaphoreGive(_i2cMutex)        // Cleanup
    );

    // Cast deviceAddress to uint8_t (I2C address)
    uint8_t addr = *static_cast<const uint8_t*>(deviceAddress);

    // Write data to the device
    RETURN_ON_ERROR_WITH_LOG(
        i2c_master_write_to_device(_i2cPort, addr, data, length, COM_TIMEOUT_MS), // Function Call
        "Failed to write data to device",                                         // Error Message
        xSemaphoreGive(_i2cMutex)                                                 // Cleanup on Error
    );

    xSemaphoreGive(_i2cMutex);

    return ERROR_SUCCESS;
}

sys_error_t com_i2c::receiveData(const void* deviceAddress, uint8_t* data, size_t maxLength, size_t& receivedLength)
{
    return ERROR_NOT_IMPLEMENTED;
}

sys_error_t com_i2c::writeRead(const void* deviceAddress, const uint8_t* writeBuffer, size_t writeSize, uint8_t* readBuffer, size_t readSize)
{
    xSemaphoreTake(_i2cMutex, portMAX_DELAY); // Take the semaphore to ensure exclusive access to I2C operations

    // Validate input arguments
    RETURN_IF_ERROR_WITH_LOG(
        (deviceAddress == nullptr || writeBuffer == nullptr || readBuffer == nullptr || writeSize < I2C_REGISTER_SIZE || readSize == 0), // Expression
        ERROR_INVALID_ARG,                                                                                                               // Error Code
        "Invalid write or read buffer or size",                                                                                          // Error Message
        xSemaphoreGive(_i2cMutex)                                                                                                        // Cleanup
    );

    // Cast deviceAddress to uint8_t (I2C address)
    uint8_t deviceAddr = *static_cast<const uint8_t*>(deviceAddress);

    // SYS_LOG_D("Writing and reading data from device at address: %p", deviceAddress);
    // Write register address and read data from the register address
    RETURN_ON_ERROR_WITH_LOG(
        i2c_master_write_read_device(_i2cPort, deviceAddr, writeBuffer, writeSize, readBuffer, readSize, COM_TIMEOUT_MS), // Function Call
        "Failed to write and read data from device",                                                                      // Error Message
        xSemaphoreGive(_i2cMutex)                                                                                         // Cleanup on Error
    );

    xSemaphoreGive(_i2cMutex); // Release the semaphore after the operation
    return ERROR_SUCCESS;
}

sys_error_t com_i2c::disconnect()
{
    return ERROR_SUCCESS;
}

sys_error_t com_i2c::setClockSpeed(uint32_t speed)
{
    if (speed == 0)
    {
        return ERROR_INVALID_ARG; // Invalid clock speed
    }

    _config.master.clk_speed = speed;

    // Reconfigure I2C with the new clock speed
    RETURN_ON_ERROR_WITH_LOG(i2c_param_config(_i2cPort, &_config), "Failed to reconfigure I2C parameters", );

    SYS_LOG_D("I2C clock speed set to %d Hz", speed);
    return ERROR_SUCCESS;
}

sys_error_t com_i2c::deInit()
{
    if (!_isInitialized)
    {
        return ERROR_SUCCESS; // Already deinitialized
    }

    xSemaphoreTake(_i2cMutex, portMAX_DELAY); // Take the mutex to ensure exclusive access

    // Uninstall the I2C driver
    RETURN_ON_ERROR_WITH_LOG(i2c_driver_delete(_i2cPort), "Failed to uninstall I2C driver", xSemaphoreGive(_i2cMutex));

    if (_i2cMutex != nullptr)
    {
        vSemaphoreDelete(_i2cMutex); // Delete the mutex
        _i2cMutex = nullptr;
    }

    _isInitialized = false; // Mark the I2C as deinitialized

    SYS_LOG_D("I2C deinitialized on port %d", _i2cPort);

    return ERROR_SUCCESS;
}