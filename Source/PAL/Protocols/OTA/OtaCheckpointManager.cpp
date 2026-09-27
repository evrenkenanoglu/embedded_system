/** @file       OtaCheckpointManager.cpp
 *  @brief      Implementation of the platform-agnostic OTA download checkpoint manager.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/09/2026
 */

/** INCLUDES ******************************************************************/

// 1. Matching Header File
#include "PAL/Protocols/OTA/OtaCheckpointManager.hpp"

// 2. Local Project / Protocol / HAL Headers
#include "Library/Common/Crc.hpp"
#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"

// 3. C++ Standard Library Headers
#include <cstring>

/** CONSTANTS *****************************************************************/

namespace
{
    constexpr size_t MIN_RESUME_CHUNK_ALIGNMENT = 4096; ///< Physical 4 KB flash sector erase boundary
} // namespace

/** FUNCTIONS *****************************************************************/

OtaCheckpointManager::OtaCheckpointManager(IHAL_MEM& storage, const std::string& keyName)
    : _storage(storage)
    , _keyName(keyName)
{
}

sys_error_t OtaCheckpointManager::save(const OtaCheckpoint_t& checkpoint)
{
    OtaCheckpoint_t payload = checkpoint;

    // 1. Compute checksum using default IEEE 802.3 profile
    const auto*  data = reinterpret_cast<const uint8_t*>(&payload);
    const size_t len  = offsetof(OtaCheckpoint_t, crc32);
    payload.crc32     = Crc32::calculate(data, len);

    // To use Castagnoli (CRC-32C) instead:
    // payload.crc32 = Crc32::calculate(data, len, Crc32::Profile::Castagnoli());

    const sys_error_t writeErr = _storage.writeData(_keyName.c_str(), reinterpret_cast<const uint8_t*>(&payload), sizeof(OtaCheckpoint_t));

    RETURN_IF_ERROR((writeErr != ERROR_SUCCESS), writeErr, SYS_LOG_E("Failed to write OTA checkpoint to storage: key '%s'", _keyName.c_str()));

    SYS_LOG_I("OTA download checkpoint saved: %zu/%zu bytes (v%s)", payload.bytesWritten, payload.targetSize, payload.targetVersion);
    return ERROR_SUCCESS;
}

sys_error_t OtaCheckpointManager::load(OtaCheckpoint_t& outCheckpoint)
{
    OtaCheckpoint_t tempCp{};

    const sys_error_t readErr = _storage.readData(_keyName.c_str(), reinterpret_cast<uint8_t*>(&tempCp), sizeof(OtaCheckpoint_t));

    RETURN_IF_ERROR(
        (readErr != ERROR_SUCCESS),                                                        // Expression
        readErr,                                                                           // Error code
        SYS_LOG_D("No active OTA checkpoint found in storage: key '%s'", _keyName.c_str()) // Error message
    );

    /// Validate CRC32 checksum against corruption from unexpected brownout during write
    const auto*    data        = reinterpret_cast<const uint8_t*>(&tempCp);
    const size_t   len         = offsetof(OtaCheckpoint_t, crc32);
    const uint32_t expectedCrc = Crc32::calculate(data, len);

    RETURN_IF_ERROR(
        (tempCp.crc32 != expectedCrc),                                                                      // Expression
        ERROR_FAIL,                                                                                         // Error code
        SYS_LOG_W("OTA checkpoint CRC32 mismatch! Expected 0x%08X, saw 0x%08X", expectedCrc, tempCp.crc32), // Error message
        clear()                                                                                             // Cleanup
    );

    outCheckpoint = tempCp;
    SYS_LOG_I("Valid OTA checkpoint loaded: %zu bytes written towards %zu bytes", outCheckpoint.bytesWritten, outCheckpoint.targetSize);
    return ERROR_SUCCESS;
}

sys_error_t OtaCheckpointManager::clear()
{
    const sys_error_t err = _storage.erase(_keyName.c_str());

    RETURN_IF_ERROR(
        (err != ERROR_SUCCESS),                                                            // Expression
        err,                                                                               // Error code
        SYS_LOG_D("Failed to erase checkpoint key '%s' (may not exist)", _keyName.c_str()) // Error message
    );

    SYS_LOG_I("OTA checkpoint cleared from storage.");
    return ERROR_SUCCESS;
}

bool OtaCheckpointManager::isResumeValid(const OtaCheckpoint_t& cp, const std::string& expectedVersion, const std::string& expectedHash, size_t expectedSize) const
{
    if (cp.bytesWritten == 0 || cp.bytesWritten >= cp.targetSize)
    {
        return false;
    }

    /// Checkpoint must align with physical flash sector erase boundary
    if (cp.bytesWritten % MIN_RESUME_CHUNK_ALIGNMENT != 0)
    {
        SYS_LOG_W("Checkpoint offset (%zu) is not sector-aligned (%zu)", cp.bytesWritten, MIN_RESUME_CHUNK_ALIGNMENT);
        return false;
    }

    if (cp.targetSize != expectedSize)
    {
        SYS_LOG_W("Checkpoint target size mismatch: %zu vs %zu", cp.targetSize, expectedSize);
        return false;
    }

    if (expectedVersion != cp.targetVersion)
    {
        SYS_LOG_W("Checkpoint version mismatch: '%s' vs '%s'", cp.targetVersion, expectedVersion.c_str());
        return false;
    }

    if (!expectedHash.empty() && expectedHash != cp.targetHash)
    {
        SYS_LOG_W("Checkpoint hash mismatch: '%s' vs '%s'", cp.targetHash, expectedHash.c_str());
        return false;
    }

    return true;
}
