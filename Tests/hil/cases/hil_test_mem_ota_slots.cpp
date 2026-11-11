/** @file       hil_test_mem_ota_slots.cpp
 *  @brief      Generic HIL contract test for the OTA application slots (ota_0 / ota_1) through IHal_Mem_Partition.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 */

/** INCLUDES ******************************************************************/
#include "hil_test_mem_ota_slots.hpp"

/** CONSTANTS *****************************************************************/
namespace
{
    constexpr const char* kOtaRunningLabel = "ota_0";
    constexpr const char* kOtaSecondLabel  = "ota_1";

    constexpr uint32_t kAppPartitionAlignment = 0x10000U; // ESP-IDF aligns OTA application partitions to 64 KB
    constexpr uint8_t  kEspImageMagic         = 0xE9U;    // First byte of every ESP-IDF application image header
    constexpr uint8_t  kErasedByte            = 0xFFU;    // Value of erased flash
    constexpr size_t   kHeaderProbeLength     = 4U;       // Bytes read from the start of each slot
} // namespace

/** FUNCTIONS *****************************************************************/
namespace
{
    sys_error_t checkSlotLayout(IHal_Mem_Partition& partitions, const char* label, hal_partition_info_t& info)
    {
        sys_error_t err = partitions.getPartitionInfo(label, &info);
        if (err != ERROR_SUCCESS)
        {
            return err;
        }

        if (info.size == 0U || (info.address % kAppPartitionAlignment) != 0U)
        {
            return ERROR_DATA_CORRUPTED;
        }

        return ERROR_SUCCESS;
    }

    bool slotsOverlap(const hal_partition_info_t& first, const hal_partition_info_t& second)
    {
        const uint32_t firstEnd  = first.address + first.size;
        const uint32_t secondEnd = second.address + second.size;
        return first.address < secondEnd && second.address < firstEnd;
    }

    sys_error_t probeSlotHeader(IHal_Mem_Partition& partitions, const char* label, bool& holdsImage)
    {
        uint8_t     header[kHeaderProbeLength] = {0};
        sys_error_t err                        = partitions.readPartition(label, 0U, header, sizeof(header));
        if (err != ERROR_SUCCESS)
        {
            return err;
        }

        if (header[0] == kEspImageMagic)
        {
            holdsImage = true;
            return ERROR_SUCCESS;
        }

        holdsImage = false;
        for (size_t i = 0U; i < sizeof(header); ++i)
        {
            if (header[i] != kErasedByte)
            {
                return ERROR_DATA_CORRUPTED;
            }
        }

        return ERROR_SUCCESS;
    }
} // namespace

sys_error_t hil_test_mem_ota_slots_contract(IHal_Mem_Partition& partitions)
{
    sys_error_t err = partitions.init();
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    hal_partition_info_t runningInfo{};
    hal_partition_info_t secondInfo{};
    err = checkSlotLayout(partitions, kOtaRunningLabel, runningInfo);
    if (err == ERROR_SUCCESS)
    {
        err = checkSlotLayout(partitions, kOtaSecondLabel, secondInfo);
    }
    if (err == ERROR_SUCCESS && slotsOverlap(runningInfo, secondInfo))
    {
        err = ERROR_DATA_CORRUPTED;
    }
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    bool runningHoldsImage = false;
    bool secondHoldsImage  = false;
    err                    = probeSlotHeader(partitions, kOtaRunningLabel, runningHoldsImage);
    if (err == ERROR_SUCCESS)
    {
        err = probeSlotHeader(partitions, kOtaSecondLabel, secondHoldsImage);
    }
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    return (runningHoldsImage || secondHoldsImage) ? ERROR_SUCCESS : ERROR_NOT_FOUND;
}
