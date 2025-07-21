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

com_i2c::com_i2c(i2c_port_t i2cPort, i2c_config_t& config)
    : _config(config)
    , _i2cPort(i2cPort)
    , _i2cMutex(nullptr)
{
    // constructor implementation
}

com_i2c::~com_i2c()
{
    // destructor implementation
}

sys_error_t com_i2c::init()
{
    // Create a mutex for I2C operations
    _i2cMutex = xSemaphoreCreateMutex();

    // Check if the mutex was created successfully
    RETURN_IF_ERROR_WITH_LOG((_i2cMutex == nullptr), ERROR_MEMORY, "Failed to create I2C mutex");

    // Initialize I2C with the provided configuration
    RETURN_ON_ERROR_WITH_LOG(i2c_param_config(_i2cPort, &_config), "Failed to configure I2C parameters", );

    // Install the I2C driver
    RETURN_ON_ERROR_WITH_LOG(i2c_driver_install(_i2cPort, _config.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0), "Failed to install I2C driver", );

    SYS_LOG_D("I2C initialized on port %d with clock speed %d", _i2cPort, _config.master.clk_speed);

    return ERROR_SUCCESS;
}

sys_error_t com_i2c::connect()
{
    return ERROR_SUCCESS;
}

sys_error_t com_i2c::sendData(const uint8_t* data, size_t length)
{

    xSemaphoreTake(_i2cMutex, portMAX_DELAY); // Take the semaphore to ensure exclusive access to I2C operations

    // Data should contain device address, register address and data to write
    // at least one byte for device address and one byte for register address and one byte for data
    // Expression | Error Code | Error Message | Cleanup
    RETURN_IF_ERROR_WITH_LOG(
        (data == nullptr || length < 2), // Expression
        ERROR_INVALID_ARG,               // Error Code
        "Invalid data buffer or length", // Error Message
        xSemaphoreGive(_i2cMutex)        // Cleanup
    );

    // extract device address, register address and data from the input buffer
    uint8_t        deviceAddress = data[0];    // assuming the first byte is the device address
    const uint8_t* writeBuffer   = data + 1;   // assuming the rest of the buffer registers and data
    size_t         writeSize     = length - 1; // subtracting the device address byte

    // Write data to the device
    RETURN_ON_ERROR_WITH_LOG(
        i2c_master_write_to_device(_i2cPort, deviceAddress, writeBuffer, writeSize, COM_TIMEOUT_MS), // Function Call
        "Failed to write data to device",                                                            // Error Message
        xSemaphoreGive(_i2cMutex)                                                                    // Release the semaphore after the operation
    );

    return ERROR_SUCCESS;
}

sys_error_t com_i2c::receiveData(uint8_t* data, size_t maxLength, size_t& receivedLength)
{
    return ERROR_NOT_IMPLEMENTED;
}

sys_error_t com_i2c::writeRead(const uint8_t* writeBuffer, size_t writeSize, uint8_t* readBuffer, size_t readSize)
{
    xSemaphoreTake(_i2cMutex, portMAX_DELAY); // Take the semaphore to ensure exclusive access to I2C operations

    // Write buffer should contain device address, register address and data to write
    RETURN_IF_ERROR_WITH_LOG((writeBuffer == nullptr || readBuffer == nullptr || writeSize < 2 || readSize == 0), ERROR_INVALID_ARG, "Invalid write or read buffer or size");

    // extract device address and register address from the write buffer
    uint8_t deviceAddress = writeBuffer[0]; // assuming the first byte is the

    // Write and read data from the device
    RETURN_ON_ERROR_WITH_LOG(
        i2c_master_write_read_device(_i2cPort, deviceAddress, writeBuffer + 1, writeSize - 1, readBuffer, readSize, COM_TIMEOUT_MS), // Function Call
        "Failed to write and read data from device",                                                                                 // Error Message
        xSemaphoreGive(_i2cMutex) // Release the semaphore after the operation
    );

    xSemaphoreGive(_i2cMutex); // Release the semaphore after the operation
    return ERROR_SUCCESS;
}

sys_error_t com_i2c::disconnect()
{
    return ERROR_SUCCESS;
}