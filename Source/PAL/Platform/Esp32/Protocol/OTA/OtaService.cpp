/** @file       OtaService.cpp
 *  @brief      Concrete implementation of the protocol-agnostic OTA update service.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       27/06/2026
 */

#include "PAL/Platform/Esp32/Protocol/OTA/OtaService.hpp"

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"

OtaService::OtaService(IOtaTransport& transport, IHal_Mem_Ota& memOta)
    : _transport(transport)
    , _memOta(memOta)
    , _state(OtaState::Idle)
    , _lastError(ERROR_SUCCESS)
    , _progressCb(nullptr)
    , _bytesWritten(0)
    , _totalSize(0)
    , _isInitialized(false)
{
}

OtaService::~OtaService()
{
    deInit();
}

sys_error_t OtaService::init()
{
    RETURN_IF_ERROR((_isInitialized), ERROR_SUCCESS);

    _state.store(OtaState::Idle);
    _lastError     = ERROR_SUCCESS;
    _bytesWritten  = 0;
    _totalSize     = 0;
    _isInitialized = true;

    return ERROR_SUCCESS;
}

sys_error_t OtaService::deInit()
{
    RETURN_IF_ERROR((!_isInitialized), ERROR_SUCCESS);

    if (_state.load() == OtaState::Downloading)
    {
        abortUpdate();
    }

    _isInitialized = false;
    return ERROR_SUCCESS;
}

sys_error_t OtaService::startUpdate(const OtaOptions_t& options, OtaProgressCb_t progressCb)
{
    /// Initialization Verification
    RETURN_IF_ERROR((!_isInitialized), ERROR_NOT_INITIALIZED, SYS_LOG_E("OTA Service not initialized!"));

    _progressCb   = progressCb;
    _bytesWritten = 0;
    _totalSize    = options.chunkSize;

    _state.store(OtaState::Downloading);
    if (_progressCb)
    {
        _progressCb(_state.load(), 0, _totalSize);
    }

    /// Establish Transport Connection
    sys_error_t err = _transport.connect();
    RETURN_IF_ERROR((err != ERROR_SUCCESS), err, SYS_LOG_E("Failed to connect update transport channel!"); _state.store(OtaState::Failed));

    const size_t expectedSize = _transport.getExpectedSize();
    if (expectedSize > 0)
    {
        _totalSize = expectedSize;
    }

    /// Flash Partition Allocation
    err = _memOta.init();
    if (err != ERROR_SUCCESS)
    {
        _transport.disconnect();
        _state.store(OtaState::Failed);
        return err;
    }

    err = _memOta.begin(_totalSize);
    if (err != ERROR_SUCCESS)
    {
        _memOta.deInit();
        _transport.disconnect();
        _state.store(OtaState::Failed);
        return err;
    }

    /// Start Stream Callback Bindings
    auto streamCb = [this](const uint8_t* chunk, size_t chunkLen, bool isLastChunk) -> sys_error_t { return this->_handleTransportChunk(chunk, chunkLen, isLastChunk); };

    err = _transport.startStream(streamCb);

    /// Transport Connection Cleanup
    _transport.disconnect();

    /// Transaction Evaluation
    if (err != ERROR_SUCCESS)
    {
        _memOta.abort();
        _memOta.deInit();
        _state.store(OtaState::Failed);
        _lastError = err;
        if (_progressCb)
        {
            _progressCb(_state.load(), _bytesWritten, _totalSize);
        }
        return _lastError;
    }

    /// Validation Verification
    _state.store(OtaState::Verifying);
    if (_progressCb)
    {
        _progressCb(_state.load(), _bytesWritten, _totalSize);
    }

    err = _memOta.end();
    if (err != ERROR_SUCCESS)
    {
        _memOta.deInit();
        _state.store(OtaState::Failed);
        _lastError = err;
        if (_progressCb)
        {
            _progressCb(_state.load(), _bytesWritten, _totalSize);
        }
        return _lastError;
    }

    /// Switch Boot Execution Target
    _state.store(OtaState::Applying);
    if (_progressCb)
    {
        _progressCb(_state.load(), _bytesWritten, _totalSize);
    }

    err = _memOta.setBootPartition();
    _memOta.deInit();

    if (err != ERROR_SUCCESS)
    {
        _state.store(OtaState::Failed);
        _lastError = err;
        if (_progressCb)
        {
            _progressCb(_state.load(), _bytesWritten, _totalSize);
        }
        return _lastError;
    }

    _state.store(OtaState::Success);
    if (_progressCb)
    {
        _progressCb(_state.load(), _bytesWritten, _totalSize);
    }

    return ERROR_SUCCESS;
}

sys_error_t OtaService::abortUpdate()
{
    RETURN_IF_ERROR((_state.load() != OtaState::Downloading), ERROR_INVALID_STATE, SYS_LOG_D("Aborting requested when not downloading."));

    _state.store(OtaState::Failed);
    _memOta.abort();
    _memOta.deInit();
    _transport.stopStream();
    _transport.disconnect();
    return ERROR_SUCCESS;
}

OtaState OtaService::getState() const
{
    return _state.load();
}

sys_error_t OtaService::getLastError() const
{
    return _lastError;
}

sys_error_t OtaService::_handleTransportChunk(const uint8_t* data, size_t length, bool isLastChunk)
{
    RETURN_IF_ERROR((_state.load() != OtaState::Downloading), ERROR_INVALID_STATE, SYS_LOG_E("Chunk received in invalid state!"));

    if (data != nullptr && length > 0)
    {
        /// Dynamic Size Update
        if (_bytesWritten + length > _totalSize)
        {
            _totalSize = _bytesWritten + length;
        }

        /// Storage Execution Check
        const sys_error_t err = _memOta.write(data, length);
        RETURN_IF_ERROR((err != ERROR_SUCCESS), err, SYS_LOG_E("Failed to write buffer chunk to OTA partition!"));

        _bytesWritten += length;

        if (_progressCb)
        {
            _progressCb(OtaState::Downloading, _bytesWritten, _totalSize);
        }
    }

    if (isLastChunk)
    {
        SYS_LOG_D("OTA update data stream completed. Total bytes received: %zu", _bytesWritten);
    }

    return ERROR_SUCCESS;
}