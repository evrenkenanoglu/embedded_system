/** @file       cpx_credentialsManager.cpp
 *  @brief      Source file for the credential orchestrator and persistence manager.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       05/07/2026
 */

/** INCLUDES ******************************************************************/
#include "cpx_credentialsManager.hpp"

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"

#include <cstring>

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** LOCAL FUNCTIONS ***********************************************************/

static void keyGenerationTask(void* param)
{
    if (param == nullptr)
    {
        SYS_LOG_E("Null parameter passed to key generation task");
        vTaskDelete(NULL);
        return;
    }

    cpx_credentialsManager* manager = static_cast<cpx_credentialsManager*>(param);
    bool                    success = (manager->generateAndStoreKeys() == ERROR_SUCCESS);

    EventBits_t bits = success ? KEY_GEN_COMPLETED : KEY_GEN_FAILED;
    xEventGroupSetBits(manager->getEventGroup(), bits);

    vTaskDelete(NULL);
}

/** FUNCTIONS ************************************************* ***************/

cpx_credentialsManager::cpx_credentialsManager(IHAL_MEM& memDevice, ICryptoEngine& cryptoEngine)
    : _memDevice(memDevice)
    , _cryptoEngine(cryptoEngine)
    , _credentials()
    , _charData()
    , _keyGenTaskHandle(nullptr)
    , _credentialMngrEventGroup(xEventGroupCreate())
    , _mutex(xSemaphoreCreateMutex())
    , _isKeyGenerationNeeded(false)
{
    _credentials.privateKey.resize(KEY_PEM_BUF_SIZE, 0);
    _credentials.serverCert.resize(CERT_PEM_BUF_SIZE, 0);
    std::memset(&_charData, 0, sizeof(_charData));
}

cpx_credentialsManager::~cpx_credentialsManager()
{
    if (_mutex != nullptr)
    {
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
    }

    if (_keyGenTaskHandle != nullptr)
    {
        vTaskDelete(_keyGenTaskHandle);
        _keyGenTaskHandle = nullptr;
    }

    if (_credentialMngrEventGroup != nullptr)
    {
        vEventGroupDelete(_credentialMngrEventGroup);
        _credentialMngrEventGroup = nullptr;
    }
}

sys_error_t cpx_credentialsManager::init(void* /*params*/)
{
    return ERROR_SUCCESS;
}

sys_error_t cpx_credentialsManager::deInit()
{
    return ERROR_SUCCESS;
}

sys_error_t cpx_credentialsManager::start()
{
    SYS_LOG_I("Starting cpx_credentialsManager...");

    RETURN_IF_ERROR(
        (_mutex == nullptr),                  // Expression
        ERROR_FAIL,                           // Error code
        SYS_LOG_E("Mutex allocation failure") // Error message
    );

    RETURN_IF_ERROR(
        (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE),      // Expression
        ERROR_FAIL,                                             // Error code
        SYS_LOG_E("Failed to acquire credentials manager lock") // Error message
    );

    sys_error_t result = ERROR_SUCCESS;

    RETURN_IF_ERROR(
        (_memDevice.init() != ERROR_SUCCESS),            // Expression
        ERROR_INIT_FAILED,                               // Error code
        SYS_LOG_E("Failed to initialize memory device"), // Error message
        xSemaphoreGive(_mutex)                           // Cleanup
    );

    std::memset(_credentials.serverCert.data(), 0, _credentials.serverCert.size());
    std::memset(_credentials.privateKey.data(), 0, _credentials.privateKey.size());

    bool serverCertExist = (_memDevice.readData(CERT_NVS_NAME, _credentials.serverCert.data(), _credentials.serverCert.size()) == ERROR_SUCCESS);
    bool privateKeyExist = (_memDevice.readData(KEY_NVS_NAME, _credentials.privateKey.data(), _credentials.privateKey.size()) == ERROR_SUCCESS);

    /// Check for pre-existing key validations [2].
    if (serverCertExist && privateKeyExist && !_isKeyGenerationNeeded)
    {
        if (std::strlen(reinterpret_cast<const char*>(_credentials.serverCert.data())) > 0)
        {
            SYS_LOG_I("Valid private key assets exist in storage.");
            xEventGroupSetBits(_credentialMngrEventGroup, KEY_GEN_COMPLETED);
            xSemaphoreGive(_mutex);
            return ERROR_SUCCESS;
        }
    }

    if (_credentialMngrEventGroup != nullptr)
    {
        xEventGroupClearBits(_credentialMngrEventGroup, KEY_GEN_COMPLETED | KEY_GEN_FAILED);
    }

    /// Spawn key generation as an asynchronous low priority worker task.
    BaseType_t isTaskCreated = xTaskCreate(keyGenerationTask, "key_gen_task", 8192, this, tskIDLE_PRIORITY + 1, &_keyGenTaskHandle);

    RETURN_IF_ERROR(
        (isTaskCreated != pdPASS),                         // Expression
        ERROR_FAIL,                                        // Error code
        SYS_LOG_E("Failed to create key generation task"), // Error message
        xSemaphoreGive(_mutex)                             // Cleanup
    );

    xSemaphoreGive(_mutex);
    return result;
}

sys_error_t cpx_credentialsManager::get(void* data)
{
    RETURN_IF_ERROR(
        (data == nullptr),                // Expression
        ERROR_INVALID_ARG,                // Error code
        SYS_LOG_E("Invalid data pointer") // Error message
    );

    RETURN_IF_ERROR(
        (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE),      // Expression
        ERROR_FAIL,                                             // Error code
        SYS_LOG_E("Failed to acquire credentials manager lock") // Error message
    );

    /// Expose certificate and key payload data to requesting protocols safely [2].
    if (std::strlen(reinterpret_cast<const char*>(_credentials.privateKey.data())) > 0)
    {
        _charData.privateKeyPtr  = _credentials.privateKey.data();
        _charData.privateKeySize = std::strlen(reinterpret_cast<const char*>(_charData.privateKeyPtr)) + 1;

        _charData.serverCertPtr  = _credentials.serverCert.data();
        _charData.serverCertSize = std::strlen(reinterpret_cast<const char*>(_charData.serverCertPtr)) + 1;

        SYS_LOG_I("Exposing verified storage credentials.");
    }
    else
    {
        _charData.privateKeyPtr  = nullptr;
        _charData.privateKeySize = 0;
        _charData.serverCertPtr  = nullptr;
        _charData.serverCertSize = 0;
    }

    xSemaphoreGive(_mutex);

    *static_cast<void**>(data) = &_charData;
    return ERROR_SUCCESS;
}

sys_error_t cpx_credentialsManager::set(void* data)
{
    if (data != nullptr)
    {
        _isKeyGenerationNeeded = *static_cast<bool*>(data);
    }
    return ERROR_SUCCESS;
}

sys_error_t cpx_credentialsManager::stop()
{
    return ERROR_SUCCESS;
}

sys_error_t cpx_credentialsManager::generateAndStoreKeys()
{
    RETURN_IF_ERROR(
        (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE), // Expression
        ERROR_FAIL,                                        // Error code
        SYS_LOG_E("Failed to lock key generation task")    // Error message
    );

    RETURN_ON_ERROR(
        _generateKeys(),                          // Expression
        SYS_LOG_E("Generation sequence failed."), // Log Message
        xSemaphoreGive(_mutex)                    // Cleanup
    );

    RETURN_ON_ERROR(
        _storeKeys(),                          // Expression
        SYS_LOG_E("Storage sequence failed."), // Log Message
        xSemaphoreGive(_mutex)                 // Cleanup
    );

    _isKeyGenerationNeeded = false;
    xSemaphoreGive(_mutex);

    return ERROR_SUCCESS;
}

sys_error_t cpx_credentialsManager::_generateKeys()
{
    std::string privateKeyPem;

    /// Use decoupled CryptoEngine to generate standard asymmetric private key [1, 2].
    RETURN_ON_ERROR(
        _cryptoEngine.generateKeyPair(ICryptoEngine::KEY_TYPE_RSA_2048, privateKeyPem), // Expression
        SYS_LOG_E("Asymmetric keypair generation failed")                               // Log Message
    );

    ICryptoEngine::CertificateConfig config;
    config.subjectName     = CERT_SUBJECT_NAME;
    config.issuerName      = CERT_SUBJECT_NAME;
    config.validitySeconds = 1419120000; // 45 years validity duration

    std::string certificatePem;

    /// Use decoupled CryptoEngine to create self-signed X.509 certificate [1, 2].
    RETURN_ON_ERROR(
        _cryptoEngine.generateSelfSignedCertificate(privateKeyPem, config, certificatePem), // Expression
        SYS_LOG_E("Self-signed certificate generation failed")                              // Log Message
    );

    /// Safely copy generated string buffers into vector cache boundaries [2].
    RETURN_IF_ERROR(
        (privateKeyPem.length() >= _credentials.privateKey.size() || certificatePem.length() >= _credentials.serverCert.size()), // Expression
        ERROR_OUT_OF_MEMORY,                                                                                                     // Error code
        SYS_LOG_E("CryptoEngine returned payloads exceeding maximum PEM limits!")                                                // Error message
    );

    std::memcpy(_credentials.privateKey.data(), privateKeyPem.c_str(), privateKeyPem.length() + 1);
    std::memcpy(_credentials.serverCert.data(), certificatePem.c_str(), certificatePem.length() + 1);

    return ERROR_SUCCESS;
}

sys_error_t cpx_credentialsManager::_storeKeys()
{
    RETURN_IF_ERROR(
        (std::strlen(reinterpret_cast<const char*>(_credentials.privateKey.data())) == 0 ||
         std::strlen(reinterpret_cast<const char*>(_credentials.serverCert.data())) == 0), // Expression
        ERROR_FAIL,                                                                        // Error code
        SYS_LOG_E("Empty private key or certificate buffer, cannot write to NVS")          // Error message
    );

    RETURN_IF_ERROR(
        (_memDevice.writeData(KEY_NVS_NAME, _credentials.privateKey.data(), _credentials.privateKey.size()) != ERROR_SUCCESS), // Expression
        ERROR_FAIL,                                                                                                            // Error code
        SYS_LOG_E("Failed to write private key to dynamic storage partitions")                                                 // Error message
    );

    RETURN_IF_ERROR(
        (_memDevice.writeData(CERT_NVS_NAME, _credentials.serverCert.data(), _credentials.serverCert.size()) != ERROR_SUCCESS), // Expression
        ERROR_FAIL,                                                                                                             // Error code
        SYS_LOG_E("Failed to write server certificate to dynamic storage partitions")                                           // Error message
    );

    return ERROR_SUCCESS;
}

EventGroupHandle_t cpx_credentialsManager::getEventGroup() const
{
    return _credentialMngrEventGroup;
}