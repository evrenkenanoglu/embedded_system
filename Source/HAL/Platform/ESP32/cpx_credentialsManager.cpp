/**
 * @file cpx_credentialsManager.cpp
 * @brief Source file for cpx_credentialsManager
 *
 * This file contains definitions for the cpx_credentialsManager class and related data types and functions.
 */

#include "cpx_credentialsManager.hpp"
#include <atomic>
#include <string>
// Logger include
#include "HAL/Platform/ESP32/library/logImpl.h"

static void keyGenerationTask(void* param);

cpx_credentialsManager::cpx_credentialsManager(IHAL_MEM& memDevice)
    : _memDevice(memDevice)                                                        // Reference to the memory device for storing credentials
    , _credentials{.privateKey = std::vector<unsigned char>(KEY_PEM_BUF_SIZE, 0),  // Initialize private key buffer
                   .serverCert = std::vector<unsigned char>(CERT_PEM_BUF_SIZE, 0)} // Initialize server certificate buffer
    , _keyGenTaskHandle(nullptr)                                                   // Task handle for key generation task
    , _credentialMngrEventGroup(xEventGroupCreate())                               // Event group for credential manager events
    , _mutex(nullptr)                                                              // Mutex for thread safety
    , isKeyGenerationNeeded(false)                                                 // Flag to indicate if new key generation is needed
{
    _mutex = xSemaphoreCreateMutex(); // Create a mutex for thread safety
}

cpx_credentialsManager::~cpx_credentialsManager()
{
    if (_mutex != nullptr)
    {
        vSemaphoreDelete(_mutex); // Delete the mutex
        _mutex = nullptr;
    }

    // Stop any running key generation task
    if (_keyGenTaskHandle != nullptr)
    {
        vTaskDelete(_keyGenTaskHandle);
        _keyGenTaskHandle = nullptr;
    }

    CleanupOnError();
}

void cpx_credentialsManager::mbedtlsComponentsInit()
{
    mbedtls_pk_init(&_key);
    mbedtls_ctr_drbg_init(&_ctr_drbg);
    mbedtls_entropy_init(&_entropy);
    mbedtls_x509write_crt_init(&_cert);
    mbedtls_mpi_init(&_serial);
}

void cpx_credentialsManager::CleanupOnError()
{
    mbedtls_pk_free(&_key);
    mbedtls_ctr_drbg_free(&_ctr_drbg);
    mbedtls_entropy_free(&_entropy);
    mbedtls_x509write_crt_free(&_cert);
    mbedtls_mpi_free(&_serial);
}

sys_error_t cpx_credentialsManager::start()
{

    // Check if the mutex is created
    if (_mutex == nullptr)
    {
        logger().log(ILog::LogLevel::ERROR, "Mutex not created for cpx_credentialsManager");
        return ERROR_FAIL;
    }

    // Semaphore for thread safety
    xSemaphoreTake(_mutex, portMAX_DELAY);

    sys_error_t result = ERROR_SUCCESS;

    if (_memDevice.init() != ERROR_SUCCESS)
    {
        logger().log(ILog::LogLevel::ERROR, "Failed to initialize memory device");
        return ERROR_FAIL;
    }

    // Clear and reuse the buffer before reading
    memset(_credentials.serverCert.data(), 0, _credentials.serverCert.size());

    // Check if the key already exists and is valid
    bool serverCertExist = (_memDevice.readData(KEY_NVS_NAME, _credentials.serverCert.data(), _credentials.serverCert.size()) == ERROR_SUCCESS);
    bool privateKeyExist = (_memDevice.readData(CERT_NVS_NAME, _credentials.privateKey.data(), _credentials.privateKey.size()) == ERROR_SUCCESS);

    if (serverCertExist && privateKeyExist && !isKeyGenerationNeeded)
    {
        // Verify we have meaningful data (non-empty PEM)
        if (strlen(reinterpret_cast<const char*>(_credentials.serverCert.data())) > 0)
        {
            logger().log(ILog::LogLevel::INFO, "Valid private key exists in storage");
            xEventGroupSetBits(_credentialMngrEventGroup, KEY_GEN_COMPLETED); // Set event bit for key generation completed
            xSemaphoreGive(_mutex);
            return ERROR_SUCCESS;
        }
    }

    // Reset event bits before creating task
    if (_credentialMngrEventGroup != nullptr)
    {
        xEventGroupClearBits(_credentialMngrEventGroup, KEY_GEN_COMPLETED | KEY_GEN_FAILED);
    }

    BaseType_t isTaskCreated = xTaskCreate(keyGenerationTask, "key_gen_task",
                                           8192,                 // Stack size - RSA generation needs more stack
                                           this,                 // Pass the instance of cpx_credentialsManager
                                           tskIDLE_PRIORITY + 1, // Task priority
                                           &_keyGenTaskHandle    // Task handle
    );

    if (isTaskCreated != pdPASS)
    {
        logger().log(ILog::LogLevel::ERROR, "Failed to create key generation task");
        result = ERROR_FAIL;
    }

    // Release the mutex
    xSemaphoreGive(_mutex);
    return result;
}

void* cpx_credentialsManager::get()
{
    // Check if keys are stored
    if (strlen(reinterpret_cast<const char*>(_credentials.serverCert.data())) > 0 && strlen(reinterpret_cast<const char*>(_credentials.privateKey.data())) > 0)
    {
        logger().log(ILog::LogLevel::INFO, "Returning stored credentials");
        return static_cast<void*>(&_credentials); // Return the credentials data
    }

    // If no keys are stored, return nullptr
    logger().log(ILog::LogLevel::INFO, "No stored credentials found, returning nullptr");
    return nullptr; // No credentials available
}

void cpx_credentialsManager::set(void* data)
{
    // Set the flag to indicate that key generation is needed
    if (data != nullptr)
    {
        isKeyGenerationNeeded = *static_cast<bool*>(data);
    }
}

sys_error_t cpx_credentialsManager::stop()
{
    return ERROR_NOT_IMPLEMENTED;
}

sys_error_t cpx_credentialsManager::generateAndStoreKeys()
{
    // Take the mutex to ensure thread safety
    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE)
    {
        logger().log(ILog::LogLevel::ERROR, "Failed to take mutex for key generation");
        return ERROR_FAIL;
    }

    // Lambda to ensure mutex is released
    auto releaseGuard = [this](const sys_error_t& res)
    {
        xSemaphoreGive(_mutex); // Ensure the mutex is released
        return res;             // Return the result of the operation
    };

    // Generate new keys
    if (generateKeys() != ERROR_SUCCESS)
    {
        logger().log(ILog::LogLevel::ERROR, "Failed to generate keys");
        return releaseGuard(ERROR_FAIL);
    }

    // Store the generated keys
    if (storeKeys() != ERROR_SUCCESS)
    {
        logger().log(ILog::LogLevel::ERROR, "Failed to store keys");
        return releaseGuard(ERROR_FAIL);
    }

    isKeyGenerationNeeded = false; // Reset the flag after successful generation and storage

    logger().log(ILog::LogLevel::INFO, "Keys generated and stored successfully");
    return releaseGuard(ERROR_SUCCESS);
}

sys_error_t cpx_credentialsManager::generateKeys()
{
    // Init mbedTLS components
    mbedtlsComponentsInit();

    // Generate the keys
    logger().log(ILog::LogLevel::INFO, "Generating Certificate and Private Key...");

    // Step 1: Seed the random number generator
    if (mbedtls_ctr_drbg_seed(&_ctr_drbg, mbedtls_entropy_func, &_entropy, NULL, 0) != 0)
    {
        logger().log(ILog::LogLevel::ERROR, "Failed to seed RNG for key generation");
        CleanupOnError();
        return ERROR_FAIL;
    }

    // Step 2: Generate the RSA key
    if (mbedtls_pk_setup(&_key, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA)) != 0 || (mbedtls_rsa_gen_key(mbedtls_pk_rsa(_key), mbedtls_ctr_drbg_random, &_ctr_drbg, KEY_SIZE, 65537)) != 0)
    {
        logger().log(ILog::LogLevel::ERROR, "Failed to generate RSA key");
        CleanupOnError();
        return ERROR_FAIL;
    }

    // Step 3: Set up the certificate details
    mbedtls_x509write_crt_set_subject_key(&_cert, &_key);
    mbedtls_x509write_crt_set_issuer_key(&_cert, &_key); // Self-signed
    mbedtls_x509write_crt_set_subject_name(&_cert, CERT_SUBJECT_NAME);
    mbedtls_mpi_lset(&_serial, time(NULL));
    mbedtls_x509write_crt_set_version(&_cert, MBEDTLS_X509_CRT_VERSION_3);
    // mbedtls_x509write_crt_set_serial(&_cert, &_serial);
    mbedtls_x509write_crt_set_validity(&_cert, CERT_TIME_STAMP_BEGIN, CERT_TIME_STAMP_END);
    mbedtls_x509write_crt_set_basic_constraints(&_cert, 1, -1);
    mbedtls_x509write_crt_set_key_usage(&_cert, MBEDTLS_X509_KU_DIGITAL_SIGNATURE);
    mbedtls_x509write_crt_set_md_alg(&_cert, MBEDTLS_MD_SHA256);

    // Step 4: Write the certificate and key to PEM format buffers
    if (mbedtls_x509write_crt_pem(&_cert, _credentials.serverCert.data(), _credentials.serverCert.size(), mbedtls_ctr_drbg_random, &_ctr_drbg) != 0 ||
        mbedtls_pk_write_key_pem(&_key, _credentials.privateKey.data(), _credentials.privateKey.size()) != 0)
    {
        logger().log(ILog::LogLevel::ERROR, "Failed to write private key or certificate to PEM format");
        CleanupOnError();
        return ERROR_FAIL;
    }

    // Cleanup mbedTLS components
    CleanupOnError();

    // Print the generated keys for debugging
    logger().log(ILog::LogLevel::INFO, "Generated private key: ");
    logger().log(ILog::LogLevel::INFO, reinterpret_cast<const char*>(_credentials.privateKey.data()));
    logger().log(ILog::LogLevel::INFO, "Generated server certificate: ");
    logger().log(ILog::LogLevel::INFO, reinterpret_cast<const char*>(_credentials.serverCert.data()));

    return ERROR_SUCCESS;
}

sys_error_t cpx_credentialsManager::storeKeys()
{
    // Validate we have data to store
    if (strlen(reinterpret_cast<const char*>(_credentials.serverCert.data())) == 0)
    {
        logger().log(ILog::LogLevel::ERROR, "Invalid private key data - empty string");
        return ERROR_FAIL;
    }

    if (strlen(reinterpret_cast<const char*>(_credentials.privateKey.data())) == 0)
    {
        logger().log(ILog::LogLevel::ERROR, "Invalid certificate data - empty string");
        return ERROR_FAIL;
    }

    // Store the private key in NVS
    if (_memDevice.writeData(KEY_NVS_NAME, _credentials.privateKey.data(), _credentials.privateKey.size()) != ERROR_SUCCESS)
    {
        logger().log(ILog::LogLevel::ERROR, "Failed to store private key in NVS");
        return ERROR_FAIL;
    }

    // Store the certificate in NVS
    if (_memDevice.writeData(CERT_NVS_NAME, _credentials.serverCert.data(), _credentials.serverCert.size()) != ERROR_SUCCESS)
    {
        logger().log(ILog::LogLevel::ERROR, "Failed to store server certificate in NVS");
        return ERROR_FAIL;
    }

    return ERROR_SUCCESS;
}

EventGroupHandle_t cpx_credentialsManager::getEventGroup() const
{
    return _credentialMngrEventGroup;
}

// Key generation task
static void keyGenerationTask(void* param)
{
    if (param == nullptr)
    {
        logger().log(ILog::LogLevel::ERROR, "Null parameter passed to key generation task");
        vTaskDelete(NULL);
        return;
    }

    cpx_credentialsManager* manager = static_cast<cpx_credentialsManager*>(param);
    bool                    success = (manager->generateAndStoreKeys() == ERROR_SUCCESS);

    // Set appropriate event bits - this is thread-safe
    EventBits_t bits = success ? KEY_GEN_COMPLETED : KEY_GEN_FAILED;
    xEventGroupSetBits(manager->getEventGroup(), bits);

    vTaskDelete(NULL);
}