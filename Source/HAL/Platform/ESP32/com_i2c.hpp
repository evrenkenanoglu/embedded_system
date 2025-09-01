/**
 * @file com_i2c.hpp
 * @brief Header file for com_i2c
 *
 * This file contains declarations for the com_i2c class and related data types and functions.
 */

#ifndef COM_I2C_HPP
#define COM_I2C_HPP

#include "HAL/IHal/IHal.h"
#include "driver/i2c.h"

class com_i2c : public IHAL_COM
{
private:
    // private members
    i2c_config_t&     _config;        // Reference to the I2C configuration
    uint8_t           _deviceAddress; // I2C device address
    uint32_t          _clockSpeed;    // I2C clock speed
    i2c_port_t        _i2cPort;       // I2C port number
    SemaphoreHandle_t _i2cMutex;
    bool              _isInitialized;

public:
    com_i2c(i2c_port_t i2cPort, i2c_config_t& config);
    ~com_i2c();

    sys_error_t init(void *params = nullptr) override;

    sys_error_t connect() override;

    sys_error_t sendData(const void* deviceAddress, const uint8_t* data, size_t length) override;

    sys_error_t receiveData(const void* deviceAddress, uint8_t* data, size_t maxLength, size_t& receivedLength) override;

    sys_error_t writeRead(const void* deviceAddress, const uint8_t* writeData, size_t writeSize, uint8_t* readData, size_t readSize) override;

    sys_error_t disconnect() override;

    sys_error_t deInit() override;

public: // User-defined methods

    /**
     * @brief Read data from an I2C device with a specified register address.
     *
     * OPERATION: First, the device address is sent, followed by the register address,
     * then the data is read from the device.
     *
     */
    sys_error_t writeRead(uint8_t deviceAddress, const uint8_t* writeBuffer, size_t writeSize, uint8_t* readBuffer, size_t readSize);
    /**
     * @brief Set the I2C clock speed.
     * @param speed The desired clock speed in Hz.
     *
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    sys_error_t setClockSpeed(uint32_t speed);
    /**
     * @brief Get the current I2C device address.
     *
     * @return uint8_t The current I2C device address.
     */
};
#endif /* COM_I2C_HPP */
