/** @file       MbedTlsCryptoEngine.cpp
 *  @brief      Concrete implementation source of ICryptoEngine using mbedTLS.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       05/07/2026
 */

/** INCLUDES ******************************************************************/

// 1. Matching Header File
#include "MbedTlsCryptoEngine.hpp"

// 2. Local Project / Protocol / HAL Headers
#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"

// 3. C++ Standard Library Headers
#include <cstring>
#include <ctime>
#include <vector>

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** LOCAL FUNCTIONS ***********************************************************/

/** FUNCTIONS *****************************************************************/

MbedTlsCryptoEngine::MbedTlsCryptoEngine()
    : _hashActive(false)
    , _activeHashType(HASH_TYPE_SHA_256)
{
    mbedtls_sha256_init(&_sha256Ctx);
    mbedtls_sha512_init(&_sha512Ctx);
}

MbedTlsCryptoEngine::~MbedTlsCryptoEngine()
{
    mbedtls_sha256_free(&_sha256Ctx);
    mbedtls_sha512_free(&_sha512Ctx);
}

sys_error_t MbedTlsCryptoEngine::_seedRng(EntropyContext& entropy, DrbgContext& drbg)
{
    int ret = mbedtls_ctr_drbg_seed(drbg.get(), mbedtls_entropy_func, entropy.get(), NULL, 0);

    RETURN_IF_ERROR(
        (ret != 0),                                                 // Expression
        ERROR_FAIL,                                                 // Error code
        SYS_LOG_E("Failed to seed CTR_DRBG context: -0x%04X", -ret) // Error message
    );

    return ERROR_SUCCESS;
}

sys_error_t MbedTlsCryptoEngine::generateKeyPair(KeyType type, std::string& outPrivateKeyPem)
{
    EntropyContext entropy;
    DrbgContext    drbg;
    PkContext      pk;

    /// Seed the hardware-based random number generator.
    RETURN_ON_ERROR(
        _seedRng(entropy, drbg),                                           // Expression
        SYS_LOG_E("Failed to seed hardware-based random number generator") // Log message
    );

    int ret = 0;

    /// Map key types and execute asymmetric key generation.
    if (type == KEY_TYPE_RSA_2048)
    {
        ret = mbedtls_pk_setup(pk.get(), mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
        if (ret == 0)
        {
            ret = mbedtls_rsa_gen_key(mbedtls_pk_rsa(*pk.get()), mbedtls_ctr_drbg_random, drbg.get(), 2048, 65537);
        }
    }
    else if (type == KEY_TYPE_EC_SECP256R1)
    {
        ret = mbedtls_pk_setup(pk.get(), mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
        if (ret == 0)
        {
            ret = mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(*pk.get()), mbedtls_ctr_drbg_random, drbg.get());
        }
    }
    else
    {
        return ERROR_INVALID_ARG;
    }

    RETURN_IF_ERROR(
        (ret != 0),                                                  // Expression
        ERROR_FAIL,                                                  // Error code
        SYS_LOG_E("Asymmetric key generation failed: -0x%04X", -ret) // Error message
    );

    /// Serialize generated asymmetric key context to PEM format [2].
    std::vector<unsigned char> writeBuffer(2048, 0);
    ret = mbedtls_pk_write_key_pem(pk.get(), writeBuffer.data(), writeBuffer.size());

    RETURN_IF_ERROR(
        (ret != 0),                                                           // Expression
        ERROR_FAIL,                                                           // Error code
        SYS_LOG_E("Failed to write private key to PEM buffer: -0x%04X", -ret) // Error message
    );

    outPrivateKeyPem = reinterpret_cast<char*>(writeBuffer.data());
    return ERROR_SUCCESS;
}

sys_error_t MbedTlsCryptoEngine::generateSelfSignedCertificate(const std::string& privateKeyPem, const CertificateConfig& config, std::string& outCertificatePem)
{
    EntropyContext   entropy;
    DrbgContext      drbg;
    PkContext        pk;
    CertWriteContext cert;

    /// Seed the hardware-based random number generator.
    RETURN_ON_ERROR(
        _seedRng(entropy, drbg),                                           // Expression
        SYS_LOG_E("Failed to seed dynamic RNG for certificate generation") // Log message
    );

    /// Parse signing private key to establish authentication context.
    int ret = mbedtls_pk_parse_key(
        pk.get(), reinterpret_cast<const unsigned char*>(privateKeyPem.c_str()), privateKeyPem.length() + 1, NULL, 0, mbedtls_ctr_drbg_random, drbg.get());

    RETURN_IF_ERROR(
        (ret != 0),                                                         // Expression
        ERROR_FAIL,                                                         // Error code
        SYS_LOG_E("Failed to parse signing private key PEM: -0x%04X", -ret) // Error message
    );

    /// Configure self-signed subject and issuer public parameters.
    mbedtls_x509write_crt_set_subject_key(cert.get(), pk.get());
    mbedtls_x509write_crt_set_issuer_key(cert.get(), pk.get());

    ret = mbedtls_x509write_crt_set_subject_name(cert.get(), config.subjectName.c_str());
    if (ret == 0)
    {
        ret = mbedtls_x509write_crt_set_issuer_name(cert.get(), config.issuerName.c_str());
    }

    RETURN_IF_ERROR(
        (ret != 0),                                                 // Expression
        ERROR_FAIL,                                                 // Error code
        SYS_LOG_E("Failed to set certificate names: -0x%04X", -ret) // Error message
    );

    /// Generate an 8-byte dynamic serial number using standard epoch timestamp.
    unsigned char serial_raw[8] = {0};
    time_t        current_time  = time(NULL);
    std::memcpy(serial_raw, &current_time, sizeof(current_time));
    mbedtls_x509write_crt_set_serial_raw(cert.get(), serial_raw, sizeof(current_time));

    /// Convert calendar dates using secure standard library timezone conversions.
    char start_date[16], end_date[16];
    std::strftime(start_date, sizeof(start_date), "%Y%m%d%H%M%S", std::gmtime(&current_time));
    time_t end_time = current_time + config.validitySeconds;
    std::strftime(end_date, sizeof(end_date), "%Y%m%d%H%M%S", std::gmtime(&end_time));
    mbedtls_x509write_crt_set_validity(cert.get(), start_date, end_date);

    mbedtls_x509write_crt_set_basic_constraints(cert.get(), 0, -1);
    mbedtls_x509write_crt_set_key_usage(cert.get(), MBEDTLS_X509_KU_DIGITAL_SIGNATURE | MBEDTLS_X509_KU_KEY_ENCIPHERMENT);
    mbedtls_x509write_crt_set_md_alg(cert.get(), MBEDTLS_MD_SHA256);

    /// Write output data payload to PEM certificate buffer [2].
    std::vector<unsigned char> writeBuffer(2048, 0);
    ret = mbedtls_x509write_crt_pem(cert.get(), writeBuffer.data(), writeBuffer.size(), mbedtls_ctr_drbg_random, drbg.get());

    RETURN_IF_ERROR(
        (ret != 0),                                                             // Expression
        ERROR_FAIL,                                                             // Error code
        SYS_LOG_E("Failed to write self-signed certificate PEM: -0x%04X", -ret) // Error message
    );

    outCertificatePem = reinterpret_cast<char*>(writeBuffer.data());
    return ERROR_SUCCESS;
}

sys_error_t MbedTlsCryptoEngine::getRandomBytes(uint8_t* outBuffer, size_t len)
{
    EntropyContext entropy;
    DrbgContext    drbg;

    /// Seed the hardware-based random number generator.
    RETURN_ON_ERROR(
        _seedRng(entropy, drbg),                                   // Expression
        SYS_LOG_E("Failed to seed RNG for random byte generation") // Log message
    );

    /// Read securely generated random bytes from seeded RNG.
    int ret = mbedtls_ctr_drbg_random(drbg.get(), outBuffer, len);

    RETURN_IF_ERROR(
        (ret != 0),                                                                // Expression
        ERROR_FAIL,                                                                // Error code
        SYS_LOG_E("Failed to read securely generated random bytes: -0x%04X", -ret) // Error message
    );

    return ERROR_SUCCESS;
}

sys_error_t MbedTlsCryptoEngine::aesGcmEncrypt(
    const uint8_t* key, size_t keyLen, const uint8_t* iv, size_t ivLen, const uint8_t* aad, size_t aadLen, const uint8_t* plaintext, size_t plaintextLen,
    uint8_t* outCiphertext, uint8_t* outTag)
{
    GcmContext gcm;

    /// Map AES hardware context and set symmetric key.
    int ret = mbedtls_gcm_setkey(gcm.get(), MBEDTLS_CIPHER_ID_AES, key, static_cast<unsigned int>(keyLen * 8));

    RETURN_IF_ERROR(
        (ret != 0),                                                            // Expression
        ERROR_FAIL,                                                            // Error code
        SYS_LOG_E("Failed to map GCM symmetric key parameters: -0x%04X", -ret) // Error message
    );

    /// Execute GCM authenticated encryption.
    ret = mbedtls_gcm_crypt_and_tag(gcm.get(), MBEDTLS_GCM_ENCRYPT, plaintextLen, iv, ivLen, aad, aadLen, plaintext, outCiphertext, 16, outTag);

    RETURN_IF_ERROR(
        (ret != 0),                                                     // Expression
        ERROR_FAIL,                                                     // Error code
        SYS_LOG_E("GCM authenticated encryption failed: -0x%04X", -ret) // Error message
    );

    return ERROR_SUCCESS;
}

sys_error_t MbedTlsCryptoEngine::aesGcmDecrypt(
    const uint8_t* key, size_t keyLen, const uint8_t* iv, size_t ivLen, const uint8_t* aad, size_t aadLen, const uint8_t* ciphertext, size_t ciphertextLen, const uint8_t* tag,
    uint8_t* outPlaintext)
{
    GcmContext gcm;

    /// Map AES hardware context and set symmetric key.
    int ret = mbedtls_gcm_setkey(gcm.get(), MBEDTLS_CIPHER_ID_AES, key, static_cast<unsigned int>(keyLen * 8));

    RETURN_IF_ERROR(
        (ret != 0),                                                            // Expression
        ERROR_FAIL,                                                            // Error code
        SYS_LOG_E("Failed to map GCM symmetric key parameters: -0x%04X", -ret) // Error message
    );

    /// Execute GCM authenticated decryption and verify the integrity tag [1].
    ret = mbedtls_gcm_auth_decrypt(gcm.get(), ciphertextLen, iv, ivLen, aad, aadLen, tag, 16, ciphertext, outPlaintext);

    RETURN_IF_ERROR(
        (ret != 0),                                                            // Expression
        ERROR_FAIL,                                                            // Error code
        SYS_LOG_W("GCM decryption authentication check failed: -0x%04X", -ret) // Error message
    );

    return ERROR_SUCCESS;
}

sys_error_t MbedTlsCryptoEngine::computeHash(HashType type, const uint8_t* data, size_t len, uint8_t* outHash)
{
    int ret = 0;

    /// Map hashing type to corresponding algorithms.
    if (type == HASH_TYPE_SHA_256)
    {
        ret = mbedtls_sha256(data, len, outHash, 0);
    }
    else if (type == HASH_TYPE_SHA_512)
    {
        ret = mbedtls_sha512(data, len, outHash, 0);
    }
    else
    {
        return ERROR_INVALID_ARG;
    }

    RETURN_IF_ERROR(
        (ret != 0),                                         // Expression
        ERROR_FAIL,                                         // Error code
        SYS_LOG_E("Hash calculation failed: -0x%04X", -ret) // Error message
    );

    return ERROR_SUCCESS;
}

sys_error_t MbedTlsCryptoEngine::computeHmac(HashType type, const uint8_t* key, size_t keyLen, const uint8_t* data, size_t dataLen, uint8_t* outMac)
{
    MdContext md;

    mbedtls_md_type_t        mdType = (type == HASH_TYPE_SHA_256) ? MBEDTLS_MD_SHA256 : MBEDTLS_MD_SHA512;
    const mbedtls_md_info_t* mdInfo = mbedtls_md_info_from_type(mdType);

    RETURN_IF_ERROR(
        (mdInfo == nullptr),                                    // Expression
        ERROR_FAIL,                                             // Error code
        SYS_LOG_E("Failed to find message digest info context") // Error message
    );

    /// Initialize Message Digest interface in HMAC mode.
    int ret = mbedtls_md_setup(md.get(), mdInfo, 1);

    RETURN_IF_ERROR(
        (ret != 0),                                         // Expression
        ERROR_FAIL,                                         // Error code
        SYS_LOG_E("mbedtls_md_setup failed: -0x%04X", -ret) // Error message
    );

    /// Compute HMAC on message bytes.
    ret = mbedtls_md_hmac_starts(md.get(), key, keyLen);
    if (ret == 0)
    {
        ret = mbedtls_md_hmac_update(md.get(), data, dataLen);
    }
    if (ret == 0)
    {
        ret = mbedtls_md_hmac_finish(md.get(), outMac);
    }

    RETURN_IF_ERROR(
        (ret != 0),                                         // Expression
        ERROR_FAIL,                                         // Error code
        SYS_LOG_E("HMAC calculation failed: -0x%04X", -ret) // Error message
    );

    return ERROR_SUCCESS;
}

sys_error_t MbedTlsCryptoEngine::hashStart(HashType type)
{
    RETURN_IF_ERROR(
        (_hashActive),                                                  // Expression
        ERROR_INVALID_STATE,                                            // Error code
        SYS_LOG_E("Another progressive hash session is already active") // Error message
    );

    int ret         = 0;
    _activeHashType = type;

    if (type == HASH_TYPE_SHA_256)
    {
        ret = mbedtls_sha256_starts(&_sha256Ctx, 0);
    }
    else if (type == HASH_TYPE_SHA_512)
    {
        ret = mbedtls_sha512_starts(&_sha512Ctx, 0);
    }
    else
    {
        return ERROR_INVALID_ARG;
    }

    RETURN_IF_ERROR(
        (ret != 0),                                                          // Expression
        ERROR_FAIL,                                                          // Error code
        SYS_LOG_E("Failed to start progressive hash context: -0x%04X", -ret) // Error message
    );

    _hashActive = true;
    return ERROR_SUCCESS;
}

sys_error_t MbedTlsCryptoEngine::hashUpdate(const uint8_t* data, size_t len)
{
    RETURN_IF_ERROR(
        (!_hashActive),                                         // Expression
        ERROR_INVALID_STATE,                                    // Error code
        SYS_LOG_E("Progressive hash calculation is not active") // Error message
    );

    int ret = 0;
    if (_activeHashType == HASH_TYPE_SHA_256)
    {
        ret = mbedtls_sha256_update(&_sha256Ctx, data, len);
    }
    else if (_activeHashType == HASH_TYPE_SHA_512)
    {
        ret = mbedtls_sha512_update(&_sha512Ctx, data, len);
    }

    RETURN_IF_ERROR(
        (ret != 0),                                                   // Expression
        ERROR_FAIL,                                                   // Error code
        SYS_LOG_E("Failed to update progressive hash: -0x%04X", -ret) // Error message
    );

    return ERROR_SUCCESS;
}

sys_error_t MbedTlsCryptoEngine::hashFinish(uint8_t* outHash)
{
    RETURN_IF_ERROR(
        (!_hashActive),                                                     // Expression
        ERROR_INVALID_STATE,                                                // Error code
        SYS_LOG_E("Cannot finish hash: progressive hashing is not active.") // Error message
    );

    int ret = 0;
    if (_activeHashType == HASH_TYPE_SHA_256)
    {
        ret = mbedtls_sha256_finish(&_sha256Ctx, outHash);
    }
    else if (_activeHashType == HASH_TYPE_SHA_512)
    {
        ret = mbedtls_sha512_finish(&_sha512Ctx, outHash);
    }

    _hashActive = false;

    RETURN_IF_ERROR(
        (ret != 0),                                                   // Expression
        ERROR_FAIL,                                                   // Error code
        SYS_LOG_E("Failed to finish progressive hash: -0x%04X", -ret) // Error message
    );

    return ERROR_SUCCESS;
}

sys_error_t MbedTlsCryptoEngine::signHash(KeyType type, const std::string& privateKeyPem, const uint8_t* hash, size_t hashLen, uint8_t* outSignature, size_t& outSignatureLen)
{
    EntropyContext entropy;
    DrbgContext    drbg;
    PkContext      pk;

    /// Seed the hardware-based random number generator.
    RETURN_ON_ERROR(
        _seedRng(entropy, drbg),                                         // Expression
        SYS_LOG_E("Failed to seed RNG for dynamic signature generation") // Log message
    );

    /// Parse signing private key to establish authentication context.
    int ret = mbedtls_pk_parse_key(
        pk.get(), reinterpret_cast<const unsigned char*>(privateKeyPem.c_str()), privateKeyPem.length() + 1, NULL, 0, mbedtls_ctr_drbg_random, drbg.get());

    RETURN_IF_ERROR(
        (ret != 0),                                             // Expression
        ERROR_FAIL,                                             // Error code
        SYS_LOG_E("Failed to parse private key: -0x%04X", -ret) // Error message
    );

    mbedtls_md_type_t mdType = (hashLen == 32) ? MBEDTLS_MD_SHA256 : MBEDTLS_MD_SHA512;

    /// Sign pre-calculated hash.
    ret = mbedtls_pk_sign(pk.get(), mdType, hash, hashLen, outSignature, 1024, &outSignatureLen, mbedtls_ctr_drbg_random, drbg.get());

    RETURN_IF_ERROR(
        (ret != 0),                                        // Expression
        ERROR_FAIL,                                        // Error code
        SYS_LOG_E("mbedtls_pk_sign failed: -0x%04X", -ret) // Error message
    );

    return ERROR_SUCCESS;
}

sys_error_t
MbedTlsCryptoEngine::verifySignature(KeyType type, const std::string& publicKeyPemOrCert, const uint8_t* hash, size_t hashLen, const uint8_t* signature, size_t signatureLen)
{
    PkContext         pk;
    mbedtls_md_type_t mdType = (hashLen == 32) ? MBEDTLS_MD_SHA256 : MBEDTLS_MD_SHA512;

    /// Parse the target key as a raw public key [2].
    int ret = mbedtls_pk_parse_public_key(pk.get(), reinterpret_cast<const unsigned char*>(publicKeyPemOrCert.c_str()), publicKeyPemOrCert.length() + 1);
    if (ret == 0)
    {
        /// Verify calculated hash against public key context.
        ret = mbedtls_pk_verify(pk.get(), mdType, hash, hashLen, signature, signatureLen);
    }
    else
    {
        /// Fallback: Parse the key as an X.509 certificate if raw public key parsing fails [2].
        mbedtls_x509_crt cert;
        mbedtls_x509_crt_init(&cert);

        ret = mbedtls_x509_crt_parse(&cert, reinterpret_cast<const unsigned char*>(publicKeyPemOrCert.c_str()), publicKeyPemOrCert.length() + 1);
        if (ret == 0)
        {
            /// Verify calculated hash against certificate context.
            ret = mbedtls_pk_verify(&cert.pk, mdType, hash, hashLen, signature, signatureLen);
        }

        mbedtls_x509_crt_free(&cert);
    }

    RETURN_IF_ERROR(
        (ret != 0),                                                   // Expression
        ERROR_FAIL,                                                   // Error code
        SYS_LOG_E("Signature check validation failed: -0x%04X", -ret) // Error message
    );

    return ERROR_SUCCESS;
}

sys_error_t MbedTlsCryptoEngine::verifyCertificateChain(const std::string& rootCaPem, const std::string& signingCertPem)
{
    mbedtls_x509_crt rootCa;
    mbedtls_x509_crt signingCert;
    mbedtls_x509_crt_init(&rootCa);
    mbedtls_x509_crt_init(&signingCert);

    /// Parse the trusted Root CA anchor certificate.
    int ret = mbedtls_x509_crt_parse(&rootCa, reinterpret_cast<const unsigned char*>(rootCaPem.c_str()), rootCaPem.length() + 1);
    if (ret != 0)
    {
        mbedtls_x509_crt_free(&rootCa);
        mbedtls_x509_crt_free(&signingCert);
        SYS_LOG_E("Failed to parse Root CA certificate: -0x%04X", -ret);
        return ERROR_FAIL;
    }

    /// Parse the incoming dynamic firmware-signing certificate [2].
    ret = mbedtls_x509_crt_parse(&signingCert, reinterpret_cast<const unsigned char*>(signingCertPem.c_str()), signingCertPem.length() + 1);
    if (ret != 0)
    {
        mbedtls_x509_crt_free(&rootCa);
        mbedtls_x509_crt_free(&signingCert);
        SYS_LOG_E("Failed to parse signing certificate: -0x%04X", -ret);
        return ERROR_FAIL;
    }

    /// Perform chain verification check [2].
    uint32_t flags = 0;
    ret            = mbedtls_x509_crt_verify(&signingCert, &rootCa, nullptr, nullptr, &flags, nullptr, nullptr);

    mbedtls_x509_crt_free(&rootCa);
    mbedtls_x509_crt_free(&signingCert);

    RETURN_IF_ERROR(
        (ret != 0 || flags != 0),                                                                         // Expression
        ERROR_FAIL,                                                                                       // Error code
        SYS_LOG_E("X.509 chain verification check failed! -0x%04X (flags: 0x%08" PRIx32 ")", -ret, flags) // Error message
    );

    return ERROR_SUCCESS;
}