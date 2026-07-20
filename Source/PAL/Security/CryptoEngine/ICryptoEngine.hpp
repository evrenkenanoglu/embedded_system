/** @file       ICryptoEngine.hpp
 *  @brief      Generic abstract cryptographic hardware/software driver interface.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       05/07/2026
 */

#pragma once

// 1. Local Project / Protocol / HAL Headers
#include "System/system.h"

// 2. C++ Standard Library Headers
#include <string>

/**
 * @class ICryptoEngine
 * @brief Generic abstract class defining cryptographic functions for asymmetric, symmetric, and hashing algorithms.
 *
 * @note Thread-Safety: Implementations of this interface must be stateless or reentrant,
 *       allowing secure concurrent execution from multiple task contexts.
 */
class ICryptoEngine
{
public:
    enum KeyType
    {
        KEY_TYPE_RSA_2048,
        KEY_TYPE_EC_SECP256R1,
        KEY_TYPE_AES_256
    };

    enum HashType
    {
        HASH_TYPE_SHA_256,
        HASH_TYPE_SHA_512
    };

    struct CertificateConfig
    {
        std::string subjectName;     // e.g., "CN=ESP32-Server,O=Universe"
        std::string issuerName;      // Same as subjectName for self-signed certificates
        uint32_t    validitySeconds; // Validity period in seconds
    };

public:
    virtual ~ICryptoEngine() = default;

    /*========================================================================*/
    /* 1. ASYMMETRIC KEY & CERTIFICATE GENERATION                             */
    /*========================================================================*/

    /**
     * @brief Generates an asymmetric private key.
     *
     * @param[in]  type            The target cryptographic algorithm.
     * @param[out] outPrivateKeyPem String containing the PEM-encoded private key.
     *
     * @return sys_error_t         ERROR_SUCCESS on success, otherwise an error code.
     *
     * @note This method must not be called from an ISR context.
     */
    virtual sys_error_t generateKeyPair(KeyType type, std::string& outPrivateKeyPem) = 0;

    /**
     * @brief Generates an X.509 self-signed certificate.
     *
     * @param[in]  privateKeyPem   PEM-encoded private key used for self-signing.
     * @param[in]  config          Distinguished Names and validity timestamps.
     * @param[out] outCertificatePem String containing the PEM-encoded certificate.
     *
     * @return sys_error_t         ERROR_SUCCESS on success, otherwise an error code.
     *
     * @note This method must not be called from an ISR context.
     */
    virtual sys_error_t generateSelfSignedCertificate(const std::string& privateKeyPem, const CertificateConfig& config, std::string& outCertificatePem) = 0;

    /*========================================================================*/
    /* 2. CRYPTOGRAPHIC RANDOM NUMBER GENERATION (CSPRNG)                    */
    /*========================================================================*/

    /**
     * @brief Generates cryptographically strong random bytes.
     *
     * @param[out] outBuffer       Pointer to the target destination buffer.
     * @param[in]  len             Number of bytes to generate.
     *
     * @return sys_error_t         ERROR_SUCCESS on success.
     */
    virtual sys_error_t getRandomBytes(uint8_t* outBuffer, size_t len) = 0;

    /*========================================================================*/
    /* 3. SYMMETRIC ENCRYPTION & DECRYPTION (AES-GCM Authenticated Encryption)*/
    /*========================================================================*/

    /**
     * @brief Encrypts plaintext using AES-GCM (Authenticated Encryption).
     *
     * @param[in]  key             Raw key buffer.
     * @param[in]  keyLen          Size of the key in bytes.
     * @param[in]  iv              Initialization Vector buffer.
     * @param[in]  ivLen           Size of the IV in bytes.
     * @param[in]  aad             Additional Authenticated Data buffer.
     * @param[in]  aadLen          Size of the AAD in bytes.
     * @param[in]  plaintext       Plaintext data payload to encrypt.
     * @param[in]  plaintextLen    Size of plaintext in bytes.
     * @param[out] outCiphertext   Destination buffer for encrypted payload.
     * @param[out] outTag          Target buffer to store the generated 16-byte authentication tag.
     *
     * @return sys_error_t         ERROR_SUCCESS on success.
     */
    virtual sys_error_t aesGcmEncrypt(
        const uint8_t* key, size_t keyLen, const uint8_t* iv, size_t ivLen, const uint8_t* aad, size_t aadLen, const uint8_t* plaintext, size_t plaintextLen,
        uint8_t* outCiphertext, uint8_t* outTag) = 0;

    /**
     * @brief Decrypts ciphertext and verifies integrity tag using AES-GCM.
     *
     * @param[in]  key             Raw key buffer.
     * @param[in]  keyLen          Size of the key in bytes.
     * @param[in]  iv              Initialization Vector buffer.
     * @param[in]  ivLen           Size of the IV in bytes.
     * @param[in]  aad             Additional Authenticated Data buffer.
     * @param[in]  aadLen          Size of the AAD in bytes.
     * @param[in]  ciphertext      Ciphertext data payload to decrypt.
     * @param[in]  ciphertextLen   Size of ciphertext in bytes.
     * @param[in]  tag             The 16-byte authentication tag to verify.
     * @param[out] outPlaintext    Destination buffer for decrypted payload.
     *
     * @return sys_error_t         ERROR_SUCCESS on success, ERROR_AUTH_FAILED if the tag does not validate.
     */
    virtual sys_error_t aesGcmDecrypt(
        const uint8_t* key, size_t keyLen, const uint8_t* iv, size_t ivLen, const uint8_t* aad, size_t aadLen, const uint8_t* ciphertext, size_t ciphertextLen,
        const uint8_t* tag, uint8_t* outPlaintext) = 0;

    /*========================================================================*/
    /* 4. INTEGRITY HASHING & MESSAGE CODES (SHA / HMAC)                      */
    /*========================================================================*/

    /**
     * @brief Computes a standard cryptographic hash of a data block.
     *
     * @param[in]  type            The hashing algorithm to use.
     * @param[in]  data            Raw message bytes buffer.
     * @param[in]  len             Length of message bytes buffer.
     * @param[out] outHash         Target destination buffer for hash (min 32 bytes for SHA-256).
     *
     * @return sys_error_t         ERROR_SUCCESS on success.
     */
    virtual sys_error_t computeHash(HashType type, const uint8_t* data, size_t len, uint8_t* outHash) = 0;

    /**
     * @brief Computes Hash-based Message Authentication Code (HMAC).
     *
     * @param[in]  type            The underlying hashing algorithm to use.
     * @param[in]  key             Raw authentication key.
     * @param[in]  keyLen          Size of key in bytes.
     * @param[in]  data            Raw message bytes.
     * @param[in]  dataLen         Size of message in bytes.
     * @param[out] outMac          Target destination buffer for verification MAC.
     *
     * @return sys_error_t         ERROR_SUCCESS on success.
     */
    virtual sys_error_t computeHmac(HashType type, const uint8_t* key, size_t keyLen, const uint8_t* data, size_t dataLen, uint8_t* outMac) = 0;

    /*========================================================================*/
    /* 5. PROGRESSIVE HASHING APIS (For Block-by-Block calculations)          */
    /*========================================================================*/

    /**
     * @brief Progressive SHA hash initialization.
     *
     * @param[in]  type            The hashing algorithm to use.
     * @return sys_error_t         ERROR_SUCCESS on success.
     */
    virtual sys_error_t hashStart(HashType type) = 0;

    /**
     * @brief Progressive SHA hash update with incoming data block.
     *
     * @param[in]  data            Raw block bytes buffer.
     * @param[in]  len             Length of data block buffer.
     * @return sys_error_t         ERROR_SUCCESS on success.
     */
    virtual sys_error_t hashUpdate(const uint8_t* data, size_t len) = 0;

    /**
     * @brief Progressive SHA hash finalization.
     *
     * @param[out] outHash         Target destination buffer for computed hash (min 32 bytes for SHA-256).
     * @return sys_error_t         ERROR_SUCCESS on success.
     */
    virtual sys_error_t hashFinish(uint8_t* outHash) = 0;

    /*========================================================================*/
    /* 6. ASYMMETRIC SIGNATURE GENERATION & VALIDATION                        */
    /*========================================================================*/

    /**
     * @brief Generates a cryptographic signature of a pre-calculated hash.
     *
     * @param[in]  type            The private key algorithm.
     * @param[in]  privateKeyPem   PEM-encoded asymmetric private key.
     * @param[in]  hash            The pre-calculated SHA hash buffer to sign.
     * @param[in]  hashLen         Size of hash buffer in bytes.
     * @param[out] outSignature    Target destination buffer for the generated signature.
     * @param[out] outSignatureLen Final generated signature length in bytes.
     *
     * @return sys_error_t         ERROR_SUCCESS on success.
     */
    virtual sys_error_t signHash(KeyType type, const std::string& privateKeyPem, const uint8_t* hash, size_t hashLen, uint8_t* outSignature, size_t& outSignatureLen) = 0;

    /**
     * @brief Verifies a hash signature using an asymmetric public certificate or key [2].
     *
     * @param[in]  type            The public key algorithm.
     * @param[in]  publicKeyPemOrCert PEM-encoded public key or X.509 certificate string.
     * @param[in]  hash            The pre-calculated SHA hash buffer.
     * @param[in]  hashLen         Size of hash buffer in bytes.
     * @param[in]  signature       Signature bytes buffer to verify.
     * @param[in]  signatureLen    Size of signature bytes buffer in bytes.
     *
     * @return sys_error_t         ERROR_SUCCESS on success, ERROR_AUTH_FAILED on mismatch.
     */
    virtual sys_error_t
    verifySignature(KeyType type, const std::string& publicKeyPemOrCert, const uint8_t* hash, size_t hashLen, const uint8_t* signature, size_t signatureLen) = 0;

    /*========================================================================*/
    /* 7. CERTIFICATE CHAIN VALIDATION                                        */
    /*========================================================================*/

    /**
     * @brief Verifies if the target certificate chains back directly to the Root CA certificate [2].
     *
     * @param[in]  rootCaPem       PEM-encoded Root CA certificate string.
     * @param[in]  signingCertPem  PEM-encoded target signing certificate string.
     * @return sys_error_t         ERROR_SUCCESS on successful validation.
     */
    virtual sys_error_t verifyCertificateChain(const std::string& rootCaPem, const std::string& signingCertPem) = 0;
};