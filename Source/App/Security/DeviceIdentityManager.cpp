/** @file       DeviceIdentityManager.cpp
 *  @brief      Runtime NVS persistence and creation for the local HTTPS identity.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 */

/** INCLUDES ******************************************************************/
#include "App/Security/DeviceIdentityManager.hpp"

#include "System/LogHandler.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <utility>

/** CONSTANTS *****************************************************************/
namespace
{
    constexpr char                    kIdentityStorageKey[] = "tlsIdentityV1";
    constexpr size_t                  kPemCapacity          = 2048U;
    constexpr size_t                  kRecordHeaderSize     = 12U;
    constexpr size_t                  kStoredRecordSize     = kRecordHeaderSize + (2U * kPemCapacity);
    constexpr std::array<uint8_t, 4U> kRecordMagic          = {'D', 'I', 'D', '1'};

    void secureZeroize(void* buffer, size_t length)
    {
        volatile uint8_t* bytes = static_cast<volatile uint8_t*>(buffer);
        for (size_t index = 0; index < length; ++index)
        {
            bytes[index] = 0U;
        }
    }

    void secureClear(std::string& value)
    {
        if (!value.empty())
        {
            secureZeroize(value.data(), value.size());
            value.clear();
        }
    }

    template <size_t Size>
    class SensitiveBuffer
    {
    public:
        std::array<uint8_t, Size> bytes{};

        ~SensitiveBuffer()
        {
            secureZeroize(bytes.data(), bytes.size());
        }

        SensitiveBuffer(const SensitiveBuffer&)            = delete;
        SensitiveBuffer& operator=(const SensitiveBuffer&) = delete;
        SensitiveBuffer()                                  = default;
    };

    uint32_t readUint32Le(const uint8_t* bytes)
    {
        return static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8U) | (static_cast<uint32_t>(bytes[2]) << 16U) | (static_cast<uint32_t>(bytes[3]) << 24U);
    }

    void writeUint32Le(uint8_t* bytes, uint32_t value)
    {
        bytes[0] = static_cast<uint8_t>(value & 0xFFU);
        bytes[1] = static_cast<uint8_t>((value >> 8U) & 0xFFU);
        bytes[2] = static_cast<uint8_t>((value >> 16U) & 0xFFU);
        bytes[3] = static_cast<uint8_t>((value >> 24U) & 0xFFU);
    }

    bool hasTerminatedPem(const uint8_t* bytes, size_t length)
    {
        constexpr char kPemBegin[] = "-----BEGIN ";
        return length > sizeof(kPemBegin) && bytes[length - 1U] == 0U && std::memchr(bytes, 0, length - 1U) == nullptr &&
               std::memcmp(bytes, kPemBegin, sizeof(kPemBegin) - 1U) == 0;
    }
} // namespace

/** FUNCTIONS *****************************************************************/
DeviceIdentity::~DeviceIdentity()
{
    secureClear(_privateKeyPem);
}

const std::string& DeviceIdentity::privateKeyPem() const
{
    return _privateKeyPem;
}

const std::string& DeviceIdentity::certificatePem() const
{
    return _certificatePem;
}

DeviceIdentityManager::DeviceIdentityManager(IHAL_MEM& storage, ICryptoEngine& cryptoEngine, const DeviceIdentityConfig& config)
    : _storage(storage)
    , _cryptoEngine(cryptoEngine)
    , _config(config)
    , _identity()
    , _initialized(false)
{
}

sys_error_t DeviceIdentityManager::initialize()
{
    if (_initialized)
    {
        return ERROR_SUCCESS;
    }

    RETURN_IF_ERROR(
        (_config.certificateSubject.empty() || _config.certificateValiditySeconds == 0U), ERROR_INVALID_CONFIG, SYS_LOG_E("Device TLS identity configuration is incomplete"));

    RETURN_ON_ERROR(
        _storage.init(),                                      // Expression
        SYS_LOG_E("Failed to initialize runtime NVS storage") // Log message
    );

    sys_error_t result = _loadStoredIdentity();
    if (result == ERROR_NOT_FOUND)
    {
        SYS_LOG_W("No persisted device TLS identity found; generating a new identity.");
        result = _generateAndStoreIdentity();
    }
    RETURN_ON_ERROR(
        result,                                                   // Expression
        SYS_LOG_E("Failed to load or create the device identity") // Log message
    );

    _initialized = true;
    SYS_LOG_I("Device TLS identity is ready in runtime NVS.");
    return ERROR_SUCCESS;
}

sys_error_t DeviceIdentityManager::getIdentity(const DeviceIdentity*& identity) const
{
    identity = nullptr;
    RETURN_IF_ERROR((!_initialized), ERROR_INVALID_STATE);
    identity = &_identity;
    return ERROR_SUCCESS;
}

sys_error_t DeviceIdentityManager::_loadStoredIdentity()
{
    SensitiveBuffer<kStoredRecordSize> record;
    sys_error_t                        result = _storage.readData(kIdentityStorageKey, record.bytes.data(), record.bytes.size());
    if (result != ERROR_SUCCESS)
    {
        if (result != ERROR_NOT_FOUND)
        {
            SYS_LOG_E("Failed to read the stored device identity (%d)", static_cast<int>(result));
        }
        return result;
    }

    result = _decodeStoredIdentity(record.bytes.data(), record.bytes.size());
    if (result != ERROR_SUCCESS)
    {
        return result;
    }
    result = _validateIdentity(_identity.privateKeyPem(), _identity.certificatePem());
    if (result != ERROR_SUCCESS)
    {
        secureClear(_identity._privateKeyPem);
        _identity._certificatePem.clear();
    }
    return result;
}

sys_error_t DeviceIdentityManager::_generateAndStoreIdentity()
{
    std::string privateKeyPem;
    std::string certificatePem;
    RETURN_ON_ERROR(
        _cryptoEngine.generateKeyPair(ICryptoEngine::KEY_TYPE_RSA_2048, privateKeyPem), // Expression
        SYS_LOG_E("Failed to generate device TLS private key"),                         // Log message
        secureClear(privateKeyPem)                                                      // Cleanup
    );

    ICryptoEngine::CertificateConfig certificateConfig;
    certificateConfig.subjectName     = _config.certificateSubject;
    certificateConfig.issuerName      = _config.certificateSubject;
    certificateConfig.validitySeconds = _config.certificateValiditySeconds;

    RETURN_ON_ERROR(
        _cryptoEngine.generateSelfSignedCertificate(privateKeyPem, certificateConfig, certificatePem), // Expression
        SYS_LOG_E("Failed to generate device TLS certificate"),                                        // Log message
        secureClear(privateKeyPem)                                                                     // Cleanup
    );

    const sys_error_t validationResult = _validateIdentity(privateKeyPem, certificatePem);
    if (validationResult != ERROR_SUCCESS)
    {
        secureClear(privateKeyPem);
        secureClear(certificatePem);
        return validationResult;
    }

    sys_error_t result = _storeIdentity(privateKeyPem, certificatePem);
    if (result != ERROR_SUCCESS)
    {
        secureClear(privateKeyPem);
        secureClear(certificatePem);
        return result;
    }

    _identity._privateKeyPem  = std::move(privateKeyPem);
    _identity._certificatePem = std::move(certificatePem);
    return ERROR_SUCCESS;
}

sys_error_t DeviceIdentityManager::_storeIdentity(const std::string& privateKeyPem, const std::string& certificatePem)
{
    RETURN_IF_ERROR(
        (privateKeyPem.size() + 1U > kPemCapacity || certificatePem.size() + 1U > kPemCapacity), // Expression
        ERROR_BUFFER_OVERFLOW,                                                                   // Error code
        SYS_LOG_E("Device identity exceeds the supported PEM size")                              // Log message
    );

    SensitiveBuffer<kStoredRecordSize> record;
    std::copy(kRecordMagic.begin(), kRecordMagic.end(), record.bytes.begin());
    const uint32_t privateKeyLength  = static_cast<uint32_t>(privateKeyPem.size() + 1U);
    const uint32_t certificateLength = static_cast<uint32_t>(certificatePem.size() + 1U);
    writeUint32Le(record.bytes.data() + 4U, privateKeyLength);
    writeUint32Le(record.bytes.data() + 8U, certificateLength);
    std::memcpy(record.bytes.data() + kRecordHeaderSize, privateKeyPem.c_str(), privateKeyLength);
    std::memcpy(record.bytes.data() + kRecordHeaderSize + kPemCapacity, certificatePem.c_str(), certificateLength);

    return _storage.writeData(kIdentityStorageKey, record.bytes.data(), record.bytes.size());
}

sys_error_t DeviceIdentityManager::_decodeStoredIdentity(const uint8_t* record, size_t recordSize)
{
    RETURN_IF_ERROR((record == nullptr || recordSize != kStoredRecordSize), ERROR_INVALID_ARG);
    RETURN_IF_ERROR((!std::equal(kRecordMagic.begin(), kRecordMagic.end(), record)), ERROR_DATA_CORRUPTED, SYS_LOG_E("Stored device identity has an unsupported format"));

    const uint32_t privateKeyLength  = readUint32Le(record + 4U);
    const uint32_t certificateLength = readUint32Le(record + 8U);
    RETURN_IF_ERROR((privateKeyLength > kPemCapacity || certificateLength > kPemCapacity), ERROR_DATA_CORRUPTED, SYS_LOG_E("Stored device identity lengths are invalid"));

    const uint8_t* privateKeyBytes     = record + kRecordHeaderSize;
    const uint8_t* certificateBytes    = privateKeyBytes + kPemCapacity;
    constexpr char kCertificateBegin[] = "-----BEGIN CERTIFICATE-----";
    RETURN_IF_ERROR(
        (!hasTerminatedPem(privateKeyBytes, privateKeyLength) || !hasTerminatedPem(certificateBytes, certificateLength) || certificateLength <= sizeof(kCertificateBegin) ||
         std::memcmp(certificateBytes, kCertificateBegin, sizeof(kCertificateBegin) - 1U) != 0),
        ERROR_DATA_CORRUPTED,
        SYS_LOG_E("Stored device identity is incomplete or malformed"));

    _identity._privateKeyPem.assign(reinterpret_cast<const char*>(privateKeyBytes), privateKeyLength - 1U);
    _identity._certificatePem.assign(reinterpret_cast<const char*>(certificateBytes), certificateLength - 1U);
    return ERROR_SUCCESS;
}

sys_error_t DeviceIdentityManager::_validateIdentity(const std::string& privateKeyPem, const std::string& certificatePem)
{
    if (privateKeyPem.empty() || privateKeyPem.find("-----BEGIN ") != 0U || privateKeyPem.find("-----END ") == std::string::npos ||
        certificatePem.find("-----BEGIN CERTIFICATE-----") != 0U || certificatePem.find("-----END CERTIFICATE-----") == std::string::npos)
    {
        return ERROR_DATA_CORRUPTED;
    }

    std::array<uint8_t, 32U>   challenge{};
    std::array<uint8_t, 1024U> signature{};
    size_t                     signatureLength = 0U;

    sys_error_t result = _cryptoEngine.getRandomBytes(challenge.data(), challenge.size());
    if (result == ERROR_SUCCESS)
    {
        result = _cryptoEngine.signHash(ICryptoEngine::KEY_TYPE_RSA_2048, privateKeyPem, challenge.data(), challenge.size(), signature.data(), signatureLength);
    }
    if (result == ERROR_SUCCESS)
    {
        result = _cryptoEngine.verifySignature(ICryptoEngine::KEY_TYPE_RSA_2048, certificatePem, challenge.data(), challenge.size(), signature.data(), signatureLength);
    }
    secureZeroize(signature.data(), signature.size());
    secureZeroize(challenge.data(), challenge.size());
    return result;
}
