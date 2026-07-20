/** @file       IOtaTransport.hpp
 *  @brief      Interface defining the protocol-agnostic data download layer.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       27/06/2026
 */

#pragma once

// 1. Local Project / Protocol / HAL Headers
#include "System/errorTranslateHandler.h"

// 2. C++ Standard Library Headers
#include <cstddef>
#include <cstdint>
#include <functional>

/**
 * @brief Stream callback logic for pushing incoming raw data buffers.
 * 
 * @param[in] chunk        Pointer to the raw binary buffer segment.
 * @param[in] chunkLen     Byte length of the current segment.
 * @param[in] isLastChunk  Set to true when the transaction concludes.
 * 
 * @return sys_error_t ERROR_SUCCESS if the chunk is processed successfully, otherwise an error status code.
 */
using OtaStreamCb_t = std::function<sys_error_t(const uint8_t* chunk, size_t chunkLen, bool isLastChunk)>;

/**
 * @class IOtaTransport
 * @brief Abstract interface defining a protocol-agnostic update binary transport line.
 *
 * @note Thread-Safety: Must be implemented as a safe, synchronous or asynchronous blocking pipeline
 *       bound strictly to the calling thread execution flow.
 */
class IOtaTransport
{
public:
    virtual ~IOtaTransport() = default;

    /**
     * @brief Activates the underlying network connection to the target source.
     *
     * @return sys_error_t ERROR_SUCCESS if successful, otherwise an error status code.
     */
    virtual sys_error_t connect() = 0;

    /**
     * @brief Closes network connections and frees transport socket handles.
     *
     * @return sys_error_t ERROR_SUCCESS if successful, otherwise an error status code.
     */
    virtual sys_error_t disconnect() = 0;

    /**
     * @brief Initiates data stream parsing and passes chunks to the mapping callback.
     *
     * This method blocks the calling task context until transmission is complete
     * or gets terminated prematurely.
     *
     * @param[in] callback Target execution block bound to the flash writer.
     *
     * @return sys_error_t ERROR_SUCCESS if download complete, otherwise an error status code.
     */
    virtual sys_error_t startStream(OtaStreamCb_t callback) = 0;

    /**
     * @brief Interrupts the active stream process.
     *
     * @return sys_error_t ERROR_SUCCESS if successful, otherwise an error status code.
     */
    virtual sys_error_t stopStream() = 0;

    /**
     * @brief Returns the total binary payload size extracted from server headers.
     *
     * @return size_t Length of the binary in bytes, or 0 if unknown.
     */
    virtual size_t getExpectedSize() const = 0;
};