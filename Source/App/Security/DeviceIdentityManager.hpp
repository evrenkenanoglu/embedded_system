/** @file       DeviceIdentityManager.hpp
 *  @brief      Loads or creates and persists the device's local HTTPS identity.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 */

#pragma once

#include "HAL/IHAL/IHal.h"
#include "PAL/Security/CryptoEngine/ICryptoEngine.hpp"
#include "System/system.h"

#include <cstddef>
#include <cstdint>
#include <string>

class DeviceIdentity final
{
public:
    ~DeviceIdentity();

    DeviceIdentity(const DeviceIdentity&)            = delete;
    DeviceIdentity& operator=(const DeviceIdentity&) = delete;
    DeviceIdentity(DeviceIdentity&&)                 = delete;
    DeviceIdentity& operator=(DeviceIdentity&&)      = delete;

    const std::string& privateKeyPem() const;
    const std::string& certificatePem() const;

private:
    friend class DeviceIdentityManager;

    DeviceIdentity() = default;

    std::string _privateKeyPem;
    std::string _certificatePem;
};

struct DeviceIdentityConfig
{
    std::string certificateSubject;
    uint32_t    certificateValiditySeconds;
};

/**
 * @class DeviceIdentityManager
 * @brief Application service that makes a persisted HTTPS device identity ready.
 *
 * Loads an existing identity from runtime NVS, or creates and persists a new
 * identity before reporting success. The owning composition must keep this
 * object alive while TLS users reference its credential strings.
 *
 * @note Thread-Safety: Single-owner service; call initialize() before sharing
 *       the returned identity with other components.
 */
class DeviceIdentityManager
{
public:
    DeviceIdentityManager(IHAL_MEM& storage, ICryptoEngine& cryptoEngine, const DeviceIdentityConfig& config);
    ~DeviceIdentityManager() = default;

    DeviceIdentityManager(const DeviceIdentityManager&)            = delete;
    DeviceIdentityManager& operator=(const DeviceIdentityManager&) = delete;
    DeviceIdentityManager(DeviceIdentityManager&&)                 = delete;
    DeviceIdentityManager& operator=(DeviceIdentityManager&&)      = delete;

    /**
     * @brief Load a persisted identity or generate and persist one if absent.
     * @return ERROR_SUCCESS only when both credentials are available and stored.
     */
    sys_error_t initialize();

    /**
     * @brief Return the ready identity.
     * @param[out] identity Receives a pointer valid for this manager's lifetime.
     * @return ERROR_SUCCESS when initialized, otherwise ERROR_INVALID_STATE.
     */
    sys_error_t getIdentity(const DeviceIdentity*& identity) const;

private:
    sys_error_t _loadStoredIdentity();
    sys_error_t _generateAndStoreIdentity();
    sys_error_t _storeIdentity(const std::string& privateKeyPem, const std::string& certificatePem);
    sys_error_t _decodeStoredIdentity(const uint8_t* record, size_t recordSize);
    sys_error_t _validateIdentity(const std::string& privateKeyPem, const std::string& certificatePem);

    IHAL_MEM&            _storage;
    ICryptoEngine&       _cryptoEngine;
    DeviceIdentityConfig _config;
    DeviceIdentity       _identity;
    bool                 _initialized;
};
