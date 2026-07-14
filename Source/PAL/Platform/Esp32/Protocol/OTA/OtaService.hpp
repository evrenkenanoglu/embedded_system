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
#include "PAL/Security/CryptoEngine/ICryptoEngine.hpp"

#include <atomic>

/**
 * @class OtaService
 * @brief Concrete PAL service class coordinating storage interfaces, network transports, and security validation.
 * 
 * @note Thread-Safety: This class provides internally serialized state transitions using atomic variables,
 *       protecting execution across multiple caller contexts.
 */
class OtaService : public IOtaService
{
public:
    /**
     * @brief Construct a new OtaService object.
     * 
     * @param[in] transport    Transport driver implementation for fetching data.
     * @param[in] memOta       Physical partition memory driver implementation.
     * @param[in] cryptoEngine Global cryptographic complex driver implementation [2].
     */
    OtaService(IOtaTransport& transport, IHal_Mem_Ota& memOta, ICryptoEngine& cryptoEngine);
    
    ~OtaService() override;

    sys_error_t init() override;
    sys_error_t deInit() override;
    
    /**
     * @brief Initiates the binary update transaction.
     *
     * @param[in]  options    Configuration options containing buffer sizes and signatures [2].
     * @param[in]  progressCb Callback structure to propagate system status modifications.
     * 
     * @return sys_error_t     ERROR_SUCCESS if successful, otherwise an error status code.
     */
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
     * @return sys_error_t    ERROR_SUCCESS if chunk was safely written, otherwise an error status code.
     */
    sys_error_t _handleTransportChunk(const uint8_t* data, size_t length, bool isLastChunk);

    /**
     * @brief Read partition block-by-block and compute its SHA-256 digest [2].
     * 
     * @param[out] outHash    Calculated 32-byte hash buffer.
     * @return sys_error_t    ERROR_SUCCESS on success.
     */
    sys_error_t _calculatePartitionHash(uint8_t* outHash);

    /**
     * @brief Decodes hex signature representations into raw byte formats [2].
     * 
     * @param[in]  hex        Source hex-encoded signature string.
     * @param[out] outBytes   Destination binary buffer to write.
     * @param[out] outLen     Parsed target output byte length.
     * @return sys_error_t    ERROR_SUCCESS on success.
     */
    sys_error_t _hexStringToBytes(const std::string& hex, uint8_t* outBytes, size_t& outLen);

    IOtaTransport&        _transport;
    IHal_Mem_Ota&         _memOta;
    ICryptoEngine&        _cryptoEngine; // Decoupled cryptographic driver [2]
    std::atomic<OtaState> _state;
    sys_error_t           _lastError;
    OtaProgressCb_t       _progressCb;

    size_t _bytesWritten;
    size_t _totalSize;
    bool   _isInitialized;
};