/** @file       MbedTlsCryptoEngine.hpp
 *  @brief      Concrete implementation of ICryptoEngine using mbedTLS.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       05/07/2026
 */

#pragma once

#include "PAL/Security/CryptoEngine/ICryptoEngine.hpp"
#include "System/system.h"

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/gcm.h>
#include <mbedtls/md.h>
#include <mbedtls/pk.h>
#include <mbedtls/sha256.h>
#include <mbedtls/sha512.h>
#include <mbedtls/x509_crt.h>

#include <string>

/**
 * @class MbedTlsCryptoEngine
 * @brief Concrete implementation of cryptographic primitives wrapping mbedTLS contexts.
 *
 * @note Thread-Safety: This implementation utilizes local-scope dynamic allocations for context wrappers,
 *       ensuring thread-safety and reentrancy.
 */
class MbedTlsCryptoEngine : public ICryptoEngine
{
private:
    class EntropyContext
    {
    public:
        EntropyContext()
        {
            mbedtls_entropy_init(&_ctx);
        }

        ~EntropyContext()
        {
            mbedtls_entropy_free(&_ctx);
        }

        mbedtls_entropy_context* get()
        {
            return &_ctx;
        }

    private:
        mbedtls_entropy_context _ctx;
    };

    class DrbgContext
    {
    public:
        DrbgContext()
        {
            mbedtls_ctr_drbg_init(&_ctx);
        }

        ~DrbgContext()
        {
            mbedtls_ctr_drbg_free(&_ctx);
        }

        mbedtls_ctr_drbg_context* get()
        {
            return &_ctx;
        }

    private:
        mbedtls_ctr_drbg_context _ctx;
    };

    class PkContext
    {
    public:
        PkContext()
        {
            mbedtls_pk_init(&_ctx);
        }

        ~PkContext()
        {
            mbedtls_pk_free(&_ctx);
        }

        mbedtls_pk_context* get()
        {
            return &_ctx;
        }

    private:
        mbedtls_pk_context _ctx;
    };

    class CertWriteContext
    {
    public:
        CertWriteContext()
        {
            mbedtls_x509write_crt_init(&_ctx);
        }

        ~CertWriteContext()
        {
            mbedtls_x509write_crt_free(&_ctx);
        }

        mbedtls_x509write_cert* get()
        {
            return &_ctx;
        }

    private:
        mbedtls_x509write_cert _ctx;
    };

    class GcmContext
    {
    public:
        GcmContext()
        {
            mbedtls_gcm_init(&_ctx);
        }

        ~GcmContext()
        {
            mbedtls_gcm_free(&_ctx);
        }

        mbedtls_gcm_context* get()
        {
            return &_ctx;
        }

    private:
        mbedtls_gcm_context _ctx;
    };

    class MdContext
    {
    public:
        MdContext()
        {
            mbedtls_md_init(&_ctx);
        }

        ~MdContext()
        {
            mbedtls_md_free(&_ctx);
        }

        mbedtls_md_context_t* get()
        {
            return &_ctx;
        }

    private:
        mbedtls_md_context_t _ctx;
    };

    /**
     * @brief Seeds the random number generator context.
     *
     * @param[in,out] entropy Reusable entropy accumulator context.
     * @param[in,out] drbg    RNG driver context.
     *
     * @return sys_error_t    ERROR_SUCCESS on success.
     */
    sys_error_t _seedRng(EntropyContext& entropy, DrbgContext& drbg);

public:
    MbedTlsCryptoEngine();
    ~MbedTlsCryptoEngine() override;

    sys_error_t generateKeyPair(KeyType type, std::string& outPrivateKeyPem) override;

    sys_error_t generateSelfSignedCertificate(
        const std::string&       privateKeyPem,    //
        const CertificateConfig& config,           //
        std::string&             outCertificatePem //
        ) override;

    sys_error_t getRandomBytes(uint8_t* outBuffer, size_t len) override;

    sys_error_t aesGcmEncrypt(
        const uint8_t* key, size_t keyLen,             //
        const uint8_t* iv, size_t ivLen,               //
        const uint8_t* aad, size_t aadLen,             //
        const uint8_t* plaintext, size_t plaintextLen, //
        uint8_t* outCiphertext, uint8_t* outTag        //
        ) override;

    sys_error_t aesGcmDecrypt(
        const uint8_t* key, size_t keyLen,               //
        const uint8_t* iv, size_t ivLen,                 //
        const uint8_t* aad, size_t aadLen,               //
        const uint8_t* ciphertext, size_t ciphertextLen, //
        const uint8_t* tag, uint8_t* outPlaintext        //
        ) override;

    sys_error_t computeHash(
        HashType       type,   //
        const uint8_t* data,   //
        size_t         len,    //
        uint8_t*       outHash //
        ) override;

    sys_error_t computeHmac(
        HashType       type,    //
        const uint8_t* key,     //
        size_t         keyLen,  //
        const uint8_t* data,    //
        size_t         dataLen, //
        uint8_t*       outMac   //
        ) override;

    sys_error_t signHash(
        KeyType            type,           //
        const std::string& privateKeyPem,  //
        const uint8_t*     hash,           //
        size_t             hashLen,        //
        uint8_t*           outSignature,   //
        size_t&            outSignatureLen //
        ) override;

    sys_error_t verifySignature(
        KeyType            type,               //
        const std::string& publicKeyPemOrCert, //
        const uint8_t*     hash,               //
        size_t             hashLen,            //
        const uint8_t*     signature,          //
        size_t             signatureLen        //
        ) override;
};