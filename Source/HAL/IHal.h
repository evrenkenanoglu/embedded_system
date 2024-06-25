/** @file       ihal.h
 *  @brief      Hardware Abstraction Layer Interface
 *  @copyright  (c) 2023- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       12/02/2023
 */
#ifndef FILE_IHAL_H
#define FILE_IHAL_H

#include "System/system.h"

/** INCLUDES ******************************************************************/

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

/**
 * @class IHAL_IO
 * @brief Interface for Hardware Abstraction Layer (HAL) I/O operations.
 */
class IHAL_IO
{
public:
    /**
     * @brief Get data from the I/O device.
     *
     * @param data Pointer to the buffer where the data will be stored.
     */
    virtual void get(void* data) = 0;

    /**
     * @brief Set data to the I/O device.
     *
     * @param data Pointer to the buffer containing the data to be set.
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t set(void* data) = 0;

    /**
     * @brief Destructor for IHAL_IO.
     */
    virtual ~IHAL_IO() {}
};

/**
 * @class IHAL_COM
 * @brief Interface for Hardware Abstraction Layer (HAL) communication operations.
 */
class IHAL_COM
{
public:
    /**
     * @brief Connect to a remote device or network.
     *
     * @return sys_error_t The error code indicating the success or failure of the connection.
     */
    virtual sys_error_t connect() = 0;

    /**
     * @brief Send data over the communication channel.
     *
     * @param data Pointer to the data buffer to be sent.
     * @param length The length of the data to be sent.
     * @return sys_error_t The error code indicating the success or failure of the data transmission.
     */
    virtual sys_error_t sendData(const uint8_t* data, size_t length) = 0;

    /**
     * @brief Receive data from the communication channel.
     *
     * @param data Pointer to the buffer where the received data will be stored.
     * @param maxLength The maximum length of data to be received.
     * @param receivedLength Reference to store the actual received data length.
     * @return sys_error_t The error code indicating the success or failure of the data reception.
     */
    virtual sys_error_t receiveData(uint8_t* data, size_t maxLength, size_t& receivedLength) = 0;

    /**
     * @brief Disconnect from the remote device or network.
     */
    virtual sys_error_t disconnect() = 0;

    /**
     * @brief Destructor for IHAL_COM.
     */
    virtual ~IHAL_COM() {}
};

/**
 * @class IHAL_MEM
 * @brief Interface for Hardware Abstraction Layer (HAL) memory device operations.
 */
class IHAL_MEM
{
public:
    /**
     * @brief Initialize the memory device.
     *
     * @return sys_error_t True if initialization was successful, false otherwise.
     */
    virtual sys_error_t init() = 0;

    /**
     * @brief Read data from the memory device.
     *
     * @param addressOrKey The memory address or key to start reading from.
     * @param data Pointer to the buffer where the read data will be stored.
     * @param length The length of data to be read.
     * @return sys_error_t True if data read was successful, false otherwise.
     */
    virtual sys_error_t readData(void* addressOrKey, uint8_t* data, size_t length) = 0;

    /**
     * @brief Write data to the memory device.
     *
     * @param addressOrKey The memory address or key to start writing to.
     * @param data Pointer to the data buffer to be written.
     * @param length The length of data to be written.
     * @return sys_error_t True if data write was successful, false otherwise.
     */
    virtual sys_error_t writeData(void* addressOrKey, const uint8_t* data, size_t length) = 0;

    /**
     * @brief Erase the memory device (if applicable).
     *
     * @return sys_error_t True if erase operation was successful, false otherwise.
     */
    virtual sys_error_t erase(void* addressOrKey) = 0;

    /**
     * @brief Get the total size of the memory device.
     * 
     * @param size Pointer to store the total size of the memory device in bytes.
     *
     * @return sys_error_t ERROR_SUCCESS if operation is successful, false otherwise.
     */
    virtual sys_error_t getSize(uint32_t *size) = 0;

    /**
     * @brief Destructor for IHAL_MEM.
     */
    virtual ~IHAL_MEM() {}
};

/**
 * @class IHAL_CPX
 * @brief Interface for Hardware Abstraction Layer (HAL) complex operations.
 */
class IHAL_CPX
{
public:
    /**
     * @brief Start the complex operation.
     *
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t start() = 0;

    /**
     * @brief Get the data result from the complex operation.
     *
     * @return void* Pointer to the buffer containing the data result.
     */
    virtual void* get() = 0;

    /**
     * @brief Set data for the complex operation.
     *
     * @param data Pointer to the buffer containing the data to be set.
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual void set(void* data) = 0;

    /**
     * @brief Stop the complex operation.
     *
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t stop() = 0;

    /**
     * @brief Destructor for IHAL_CPX.
     */
    virtual ~IHAL_CPX() {}
};

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** FUNCTIONS *****************************************************************/

#undef INTERFACE // Should not let this roam free

#endif // FILE_IHAL_H
