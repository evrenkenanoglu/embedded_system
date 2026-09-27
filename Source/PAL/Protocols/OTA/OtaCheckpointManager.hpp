/** @file       OtaCheckpointManager.hpp
 *  @brief      Platform-agnostic OTA download checkpoint persistence manager.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/09/2026
 */

#pragma once

// 1. Local Project / Protocol / HAL Headers
#include "HAL/IHAL/IHal.h"
#include "System/system.h"

// 2. C++ Standard Library Headers
#include <cstddef>
#include <cstdint>
#include <string>

/**
 * @struct OtaCheckpoint_t
 * @brief Binary checkpoint layout persisted to non-volatile storage.
 */
struct OtaCheckpoint_t
{
    char     targetVersion[32]; ///< Target firmware semantic version string
    char     targetHash[65];    ///< Expected SHA-256 hexadecimal digest
    size_t   targetSize;        ///< Total expected application binary size in bytes
    size_t   bytesWritten;      ///< Byte offset successfully written and aligned to flash
    uint32_t crc32;             ///< Checksum protecting against partial/corrupted flash writes
};

/**
 * @class OtaCheckpointManager
 * @brief Manages non-volatile OTA download resumption checkpoints via IHAL_MEM.
 *
 * @note Thread-Safety: Not thread-safe. Synchronization must be provided by the calling service.
 */
class OtaCheckpointManager
{
private:
    IHAL_MEM&         _storage;
    const std::string _keyName;

public:
    /**
     * @brief Constructs a new OtaCheckpointManager instance.
     *
     * @param[in] storage Reference to an initialized IHAL_MEM storage driver.
     * @param[in] keyName Storage key identifier (defaults to "ota_chkpt").
     */
    explicit OtaCheckpointManager(IHAL_MEM& storage, const std::string& keyName = "ota_chkpt");

    ~OtaCheckpointManager() = default;

    // Rule of Five: Delete copy/move constructors for resource wrappers
    OtaCheckpointManager(const OtaCheckpointManager&)            = delete;
    OtaCheckpointManager& operator=(const OtaCheckpointManager&) = delete;
    OtaCheckpointManager(OtaCheckpointManager&&)                 = delete;
    OtaCheckpointManager& operator=(OtaCheckpointManager&&)      = delete;

public:
    /**
     * @brief Persists an active download checkpoint with CRC32 integrity protection.
     *
     * @param[in] checkpoint Reference to the checkpoint data to write.
     * @return sys_error_t ERROR_SUCCESS on success, otherwise an error status code.
     */
    sys_error_t save(const OtaCheckpoint_t& checkpoint);

    /**
     * @brief Loads and validates an existing checkpoint from storage.
     *
     * @param[out] outCheckpoint Populated with the stored checkpoint if valid.
     * @return sys_error_t ERROR_SUCCESS if valid, ERROR_NOT_FOUND if absent, ERROR_FAIL on corruption.
     */
    sys_error_t load(OtaCheckpoint_t& outCheckpoint);

    /**
     * @brief Purges any active checkpoint from non-volatile storage.
     *
     * @return sys_error_t ERROR_SUCCESS on successful erasure.
     */
    sys_error_t clear();

    /**
     * @brief Verifies whether a stored checkpoint matches the target firmware manifest parameters.
     *
     * @param[in] cp              The loaded checkpoint instance.
     * @param[in] expectedVersion The target version string from the server manifest.
     * @param[in] expectedHash    The expected target hash from the server manifest.
     * @param[in] expectedSize    The expected total binary length.
     * @return true if valid for resumption, false otherwise.
     */
    bool isResumeValid(const OtaCheckpoint_t& cp, const std::string& expectedVersion, const std::string& expectedHash, size_t expectedSize) const;
};
