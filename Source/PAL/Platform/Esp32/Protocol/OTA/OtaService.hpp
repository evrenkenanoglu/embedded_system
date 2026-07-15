/** @file       OtaService.hpp
 *  @brief      Concrete implementation of the protocol-agnostic OTA update service.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       27/06/2026
 */

#pragma once

#include "HAL/IHAL/IHal_Mem_Ota.h"
#include "PAL/Protocols/OTA/IOtaService.hpp"
#include "PAL/Protocols/OTA/IOtaTransport.hpp"

#include <atomic>
#include <string>

// Forward declarations
class ICryptoEngine;

/**
 * @class OtaService
 * @brief Concrete PAL service class coordinating storage interfaces, network transports, and crypto verification.
 */
class OtaService : public IOtaService
{
public:
    OtaService(IOtaTransport& transport, IHal_Mem_Ota& memOta, ICryptoEngine& cryptoEngine);
    ~OtaService() override;

    sys_error_t init() override;
    sys_error_t deInit() override;
    sys_error_t startUpdate(const OtaOptions_t& options, OtaProgressCb_t progressCb) override;
    sys_error_t abortUpdate() override;
    OtaState    getState() const override;
    sys_error_t getLastError() const override;

private:
    /**
     * @brief Stream callback handler bound to receive bytes from the transport pipeline.
     *
     * @param[in] data        Pointer to the raw buffer block.
     * @param[in] length      Byte length of the current block.
     * @param[in] isLastChunk Flag indicating if the transfer cycle is concluding.
     *
     * @return sys_error_t ERROR_SUCCESS if chunk was safely written, otherwise an error status code.
     */
    sys_error_t _handleTransportChunk(const uint8_t* data, size_t length, bool isLastChunk);

    /**
     * @brief Calculates the SHA-256 hash of the written partition progressively.
     *
     * Reads the partition block-by-block up to the exact target size and feeds it
     * to the CryptoEngine context to prevent high memory usage and Out-of-Memory faults.
     *
     * @param[in]  targetSize   The exact size of the application binary in bytes.
     * @param[out] outHash      Buffer to store the calculated SHA-256 hash (min 32 bytes).
     * @return sys_error_t ERROR_SUCCESS if successful, otherwise an error status code.
     */
    sys_error_t _calculatePartitionHash(size_t targetSize, uint8_t* outHash);

    /**
     * @brief Converts a hex string representation to raw byte buffers.
     *
     * @param[in]  hex      Hexadecimal input string.
     * @param[out] outBytes Buffer to write parsed raw bytes into.
     * @param[out] outLen   The final parsed byte count.
     * @return sys_error_t ERROR_SUCCESS if successful, otherwise ERROR_INVALID_ARG.
     */
    sys_error_t _hexStringToBytes(const std::string& hex, uint8_t* outBytes, size_t& outLen);

    IOtaTransport&        _transport;
    IHal_Mem_Ota&         _memOta;
    ICryptoEngine&        _cryptoEngine;
    std::atomic<OtaState> _state;
    sys_error_t           _lastError;
    OtaProgressCb_t       _progressCb;

    size_t _bytesWritten;
    size_t _totalSize;
    bool   _isInitialized;
};