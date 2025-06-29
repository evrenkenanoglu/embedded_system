/**
 * @file cpx_credentialsManager.hpp
 * @brief Header file for cpx_credentialsManager
 *
 * This file contains declarations for the cpx_credentialsManager class and related data types and functions.
 */

#ifndef CPX_CREDENTIALS_MANAGER_HPP
#define CPX_CREDENTIALS_MANAGER_HPP

#include "HAL/IHal.h"
#include <vector>

// mbedTLS headers
#include "mbedtls/build_info.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/pk.h"
#include "mbedtls/rsa.h"
#include "mbedtls/version.h"
#include "mbedtls/x509.h"
#include "mbedtls/x509_crt.h"

#include "esp_bit_defs.h"

#define CERT_SUBJECT_NAME     "CN=ESP32-Server,O=Universe" // Common Name, Organization
#define CERT_TIME_STAMP_BEGIN "20230101000000"             // YYYYMMDDHHMMSS format
#define CERT_TIME_STAMP_END   "20700101000000"             // YYYYMMDDHHMMSS format

#define KEY_SIZE              2048
#define KEY_NVS_NAME          "prvtkey"
#define CERT_NVS_NAME         "servercert"
#define KEY_PEM_BUF_SIZE      4096
#define CERT_PEM_BUF_SIZE     4096

#define KEY_GEN_STARTED       BIT0
#define KEY_GEN_COMPLETED     BIT1
#define KEY_GEN_FAILED        BIT2

struct CredentialsData
{
    std::vector<unsigned char> privateKey; // Private key in PEM format
    std::vector<unsigned char> serverCert; // Server certificate in PEM format
};

class cpx_credentialsManager : public IHAL_CPX
{
private:
    // private members
    IHAL_MEM&          _memDevice;  // Reference to the memory device for storing credentials
    CredentialsData    _credentials; // Struct to hold the credentials data
    TaskHandle_t       _keyGenTaskHandle;
    EventGroupHandle_t _credentialMngrEventGroup;
    SemaphoreHandle_t  _mutex;

    bool isKeyGenerationNeeded; // Flag to indicate if new key generation is needed

    // mbedTLS components
    mbedtls_pk_context       _key;
    mbedtls_entropy_context  _entropy;
    mbedtls_ctr_drbg_context _ctr_drbg;
    mbedtls_x509write_cert   _cert;
    mbedtls_mpi              _serial;

private:
    sys_error_t generateKeys();
    sys_error_t storeKeys();
    void        mbedtlsComponentsInit();
    void        CleanupOnError();

public: // Interface methods
    cpx_credentialsManager(IHAL_MEM& memDevice);
    ~cpx_credentialsManager();

    sys_error_t start() override;

    /**
     * @brief Get the stored credentials.
     *
     * @return void* pointer to the stored credentials data.
     */
    void* get() override;

    void set(void* data) override;

    sys_error_t stop() override;

public: // User-defined methods
    sys_error_t        generateAndStoreKeys();
    EventGroupHandle_t getEventGroup() const;
};
#endif /* CPX_CREDENTIALS_MANAGER_HPP */
