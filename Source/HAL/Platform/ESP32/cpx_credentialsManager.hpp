/** @file       cpx_credentialsManager.hpp
 *  @brief      Header file for the credential orchestrator and persistence manager.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       05/07/2026
 */

#pragma once

#include "HAL/IHAL/IHal.h"
#include "PAL/Security/CryptoEngine/ICryptoEngine.hpp"

#include <vector>

#define CERT_SUBJECT_NAME     "CN=ESP32-Server,O=Universe"
#define KEY_NVS_NAME          "prvtkey"
#define CERT_NVS_NAME         "servercert"
#define KEY_PEM_BUF_SIZE      2048
#define CERT_PEM_BUF_SIZE     2048

#define KEY_GEN_STARTED       (1 << 0)
#define KEY_GEN_COMPLETED     (1 << 1)
#define KEY_GEN_FAILED        (1 << 2)

struct CredentialsCharData_t
{
    uint8_t* privateKeyPtr;
    size_t   privateKeySize;
    uint8_t* serverCertPtr;
    size_t   serverCertSize;
};

/**
 * @class cpx_credentialsManager
 * @brief High-level summary of the class's responsibility to manage dynamic storage credentials.
 * 
 * @note Thread-Safety: Thread safety is guaranteed internally via a FreeRTOS mutex semaphore,
 *       protecting credentials load, generate, and store transactions.
 */
class cpx_credentialsManager : public IHAL_CPX
{
private:
    struct CredentialsData
    {
        std::vector<uint8_t> privateKey; // Private key PEM buffer
        std::vector<uint8_t> serverCert; // Certificate PEM buffer
    };

    IHAL_MEM&             _memDevice;    // NVS Hardware abstraction
    ICryptoEngine&        _cryptoEngine; // Decoupled Cryptographic driver [1]
    CredentialsData       _credentials;  // Local memory cache
    CredentialsCharData_t _charData;     // Pointers to expose to applications
    TaskHandle_t          _keyGenTaskHandle;
    EventGroupHandle_t    _credentialMngrEventGroup;
    SemaphoreHandle_t     _mutex;

    bool _isKeyGenerationNeeded;

private:
    /**
     * @brief Internal helper to trigger dynamic keypair and self-signed certificate generation [1, 2].
     * 
     * @return sys_error_t ERROR_SUCCESS on success.
     */
    sys_error_t _generateKeys();

    /**
     * @brief Writes generated credential buffers to hardware NVS flash blocks.
     * 
     * @return sys_error_t ERROR_SUCCESS on success.
     */
    sys_error_t _storeKeys();

public:
    cpx_credentialsManager(IHAL_MEM& memDevice, ICryptoEngine& cryptoEngine);
    ~cpx_credentialsManager() override;

    sys_error_t init(void* params = nullptr) override;
    sys_error_t start() override;

    /**
     * @brief Retrieves active credential pointers.
     * 
     * @param[out] data Output pointer populated with the active CredentialsCharData_t struct pointer.
     * @return sys_error_t ERROR_SUCCESS on success.
     */
    sys_error_t get(void* data) override;

    /**
     * @brief Configures key generation triggers.
     * 
     * @param[in]  data Pointer containing boolean parameter to enforce generation.
     * @return sys_error_t ERROR_SUCCESS on success.
     */
    sys_error_t set(void* data) override;

    sys_error_t stop() override;
    sys_error_t deInit() override;

    /**
     * @brief Thread-safe routine to execute dynamic key generation and write cycles.
     * 
     * @return sys_error_t ERROR_SUCCESS on success.
     */
    sys_error_t        generateAndStoreKeys();

    /**
     * @brief Retrieves the active event group handler.
     * 
     * @return EventGroupHandle_t FreeRTOS event group.
     */
    EventGroupHandle_t getEventGroup() const;
};