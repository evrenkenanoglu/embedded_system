/** @file       OtaService.cpp
 *  @brief      Concrete implementation of the protocol-agnostic OTA update service.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       27/06/2026
 */

/** INCLUDES ******************************************************************/
#include "PAL/Platform/Esp32/Protocol/OTA/OtaService.hpp"
#include "PAL/Security/CryptoEngine/ICryptoEngine.hpp"

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"

#include <cstdlib>
#include <cstring>

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** LOCAL FUNCTIONS ***********************************************************/

namespace
{
    /**
     * @brief Converts a hexadecimal string into its raw byte array representation.
     */
    bool hexToBytes(const std::string& hex, std::vector<uint8_t>& outBytes)
    {
        if (hex.length() % 2 != 0)
        {
            return false;
        }
        outBytes.clear();
        outBytes.reserve(hex.length() / 2);
        for (size_t i = 0; i < hex.length(); i += 2)
        {
            std::string byteString = hex.substr(i, 2);
            char*       endptr     = nullptr;
            long        byteVal    = std::strtol(byteString.c_str(), &endptr, 16);
            if (endptr == byteString.c_str() || *endptr != '\0')
            {
                return false;
            }
            outBytes.push_back(static_cast<uint8_t>(byteVal));
        }
        return true;
    }
} // namespace

/** FUNCTIONS *****************************************************************/

OtaService::OtaService(IOtaTransport& transport, IHal_Mem_Ota& memOta, ICryptoEngine& cryptoEngine)
    : _transport(transport)
    , _memOta(memOta)
    , _cryptoEngine(cryptoEngine)
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
    RETURN_IF_ERROR(
        (_isInitialized),                            // Expression
        ERROR_SUCCESS,                               // Error code
        SYS_LOG_I("OTA Service already initialized") // Error message
    );

    _state.store(OtaState::Idle);
    _lastError     = ERROR_SUCCESS;
    _bytesWritten  = 0;
    _totalSize     = 0;
    _isInitialized = true;

    return ERROR_SUCCESS;
}

sys_error_t OtaService::deInit()
{
    RETURN_IF_ERROR(
        (!_isInitialized),                       // Expression
        ERROR_SUCCESS,                           // Error code
        SYS_LOG_I("OTA Service not initialized") // Error message
    );

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
    RETURN_IF_ERROR(
        (!_isInitialized),                        // Expression
        ERROR_NOT_INITIALIZED,                    // Error code
        SYS_LOG_E("OTA Service not initialized!") // Error message
    );

    _progressCb   = progressCb;
    _bytesWritten = 0;
    _totalSize    = options.chunkSize;

    /// 1. Prior to downloading, verify the dynamic code signing key trust validation chain [2].
    if (!options.signingCert.empty() && !options.serverCert.empty())
    {
        _state.store(OtaState::Verifying);
        if (_progressCb)
        {
            _progressCb(_state.load(), 0, _totalSize);
        }

        /// Delegate X.509 chain verification directly to the CryptoEngine [1, 2].
        sys_error_t verifyErr = _cryptoEngine.verifyCertificateChain(options.serverCert, options.signingCert);

        RETURN_IF_ERROR((verifyErr != ERROR_SUCCESS),                                              // Expression
                        verifyErr,                                                                 // Error code
                        SYS_LOG_E("[PKI] Dynamic signing certificate chain verification failed!"), // Error message
                        _memOta.deInit();                                                          // Cleanup
                        _state.store(OtaState::Failed)                                             // Cleanup
        );

        SYS_LOG_I("[PKI] Dynamic signing certificate validated successfully against trust anchor.");
    }

    _state.store(OtaState::Downloading);
    if (_progressCb)
    {
        _progressCb(_state.load(), 0, _totalSize);
    }

    /// Establish Transport Connection.
    sys_error_t err = _transport.connect();

    RETURN_IF_ERROR(
        (err != ERROR_SUCCESS),                                   // Expression
        err,                                                      // Error code
        SYS_LOG_E("Failed to connect update transport channel!"), // Error message
        _state.store(OtaState::Failed)                            // Cleanup
    );

    const size_t expectedSize = _transport.getExpectedSize();
    if (expectedSize > 0)
    {
        _totalSize = expectedSize;
    }

    /// Flash Partition Allocation.
    err = _memOta.init();
    if (err != ERROR_SUCCESS)
    {
        _transport.disconnect();
        _state.store(OtaState::Failed);
        return err;
    }

    /// Dynamically route write target logic based on the identified download payload type
    _memOta.setDeltaMode(options.isDelta);

    err = _memOta.begin(_totalSize);
    if (err != ERROR_SUCCESS)
    {
        _memOta.deInit();
        _transport.disconnect();
        _state.store(OtaState::Failed);
        return err;
    }

    /// Start Stream Callback Bindings.
    auto streamCb = [this](const uint8_t* chunk, size_t chunkLen, bool isLastChunk) -> sys_error_t { return this->_handleTransportChunk(chunk, chunkLen, isLastChunk); };

    err = _transport.startStream(streamCb);

    /// Transport Connection Cleanup.
    _transport.disconnect();

    /// Transaction Evaluation.
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

    /// Validation Verification and Dynamic PKI verification.
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

    /// 2. If signatures are active, execute cryptographic code-sign validation [2].
    if (!options.signingCert.empty() && (!options.signature.empty() || !options.targetSignature.empty()))
    {
        // Select the appropriate target signature to verify against the reconstructed target partition
        const std::string& signatureToVerify = (options.isDelta && !options.targetSignature.empty()) ? options.targetSignature : options.signature;

        SYS_LOG_I("[PKI] Decoding HEX target signature payload...");

        uint8_t     rawSignature[128];
        size_t      sigLen = 0;
        sys_error_t hexErr = _hexStringToBytes(signatureToVerify, rawSignature, sigLen);
        if (hexErr != ERROR_SUCCESS)
        {
            _memOta.deInit();
            _state.store(OtaState::Failed);
            return hexErr;
        }

        SYS_LOG_I("[PKI] Calculating SHA-256 target partition digest...");

        uint8_t     calculatedHash[32];
        sys_error_t hashErr = _calculatePartitionHash(options.targetSize, calculatedHash);
        if (hashErr != ERROR_SUCCESS)
        {
            _memOta.deInit();
            _state.store(OtaState::Failed);
            return hashErr;
        }

        SYS_LOG_I("[PKI] Verifying cryptographic signature against public key context...");

        /// Delegate verification to the CryptoEngine, keeping memory drivers completely separate [1, 2].
        sys_error_t sigErr = _cryptoEngine.verifySignature(
            ICryptoEngine::KEY_TYPE_EC_SECP256R1, //
            options.signingCert,                  //
            calculatedHash,                       //
            sizeof(calculatedHash),               //
            rawSignature,                         //
            sigLen                                //
        );

        if (sigErr != ERROR_SUCCESS)
        {
            SYS_LOG_E("[PKI] Cryptographic signature verification failed!");
            _memOta.deInit();
            _state.store(OtaState::Failed);
            return sigErr;
        }

        SYS_LOG_I("[PKI] Dynamic certificate chain and cryptographic update signatures verified successfully.");
    }

    /// Switch Boot Execution Target.
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
    RETURN_IF_ERROR(
        (_state.load() != OtaState::Downloading),             // Expression
        ERROR_INVALID_STATE,                                  // Error code
        SYS_LOG_D("Aborting requested when not downloading.") // Error message
    );

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
    RETURN_IF_ERROR(
        (_state.load() != OtaState::Downloading),     // Expression
        ERROR_INVALID_STATE,                          // Error code
        SYS_LOG_E("Chunk received in invalid state!") // Error message
    );

    if (data != nullptr && length > 0)
    {
        /// Dynamic Size Update
        if (_bytesWritten + length > _totalSize)
        {
            _totalSize = _bytesWritten + length;
        }

        /// Storage Execution Check
        const sys_error_t err = _memOta.write(data, length);

        RETURN_IF_ERROR(
            (err != ERROR_SUCCESS),                                     // Expression
            err,                                                        // Error code
            SYS_LOG_E("Failed to write buffer chunk to OTA partition!") // Error message
        );

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

sys_error_t OtaService::_calculatePartitionHash(size_t targetSize, uint8_t* outHash)
{
    size_t partitionSize = _memOta.getPartitionSize();

    RETURN_IF_ERROR(
        (partitionSize == 0),                                   // Expression
        ERROR_INVALID_STATE,                                    // Error code
        SYS_LOG_E("Update partition size is zero, cannot hash") // Error message
    );

    /// Constrain hashing bounds using exact reassembled file size metric
    size_t sizeLeft = (targetSize > 0 && targetSize <= partitionSize) ? targetSize : partitionSize;

    /// Use the decoupled progressive hashing APIs of the ICryptoEngine [1, 2].
    RETURN_ON_ERROR(
        _cryptoEngine.hashStart(ICryptoEngine::HASH_TYPE_SHA_256), // Expression
        SYS_LOG_E("Failed to start SHA-256 calculation")           // Log message
    );

    size_t  offset = 0;
    uint8_t readBuffer[4096];

    while (sizeLeft > 0)
    {
        size_t readSize = (sizeLeft > sizeof(readBuffer)) ? sizeof(readBuffer) : sizeLeft;

        /// Direct read of physical blocks from target partition
        sys_error_t err = _memOta.read(offset, readBuffer, readSize);
        if (err != ERROR_SUCCESS)
        {
            return err;
        }

        /// Progressive SHA-256 hash calculation [1, 2]
        err = _cryptoEngine.hashUpdate(readBuffer, readSize);
        if (err != ERROR_SUCCESS)
        {
            return err;
        }

        offset += readSize;
        sizeLeft -= readSize;
    }

    return _cryptoEngine.hashFinish(outHash);
}

sys_error_t OtaService::_hexStringToBytes(const std::string& hex, uint8_t* outBytes, size_t& outLen)
{
    /// if the hex string length is odd, it's invalid
    if (hex.length() % 2 != 0)
    {
        return ERROR_INVALID_ARG;
    }

    /// Calculate the expected output length
    outLen = hex.length() / 2;

    /// Convert each pair of hex characters to a byte
    for (size_t i = 0; i < outLen; ++i)
    {
        std::string byteString = hex.substr(i * 2, 2);
        char*       endptr     = nullptr;
        long        byteVal    = std::strtol(byteString.c_str(), &endptr, 16);
        if (endptr == byteString.c_str() || *endptr != '\0')
        {
            return ERROR_INVALID_ARG;
        }
        outBytes[i] = static_cast<uint8_t>(byteVal);
    }

    return ERROR_SUCCESS;
}