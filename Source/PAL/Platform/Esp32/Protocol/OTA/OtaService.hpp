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

/**
 * @class OtaService
 * @brief Concrete PAL service class coordinating storage interfaces and network transports.
 */
class OtaService : public IOtaService
{
public:
    OtaService(IOtaTransport& transport, IHal_Mem_Ota& memOta);
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

    IOtaTransport&        _transport;
    IHal_Mem_Ota&         _memOta;
    std::atomic<OtaState> _state;
    sys_error_t           _lastError;
    OtaProgressCb_t       _progressCb;

    size_t _bytesWritten;
    size_t _totalSize;
    bool   _isInitialized;
};