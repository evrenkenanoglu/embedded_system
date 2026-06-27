/** @file       mem_ota.hpp
 *  @brief      Memory implementation
 *  @copyright  (c) 2023- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       17/06/2026
 */
#ifndef MEM_OTA_HPP
#define MEM_OTA_HPP

#include "HAL/IHAL/IHal_Mem_Ota.h"
#include "esp_ota_ops.h"

/** INCLUDES ******************************************************************/

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

/**
 * @class mem_ota
 * @brief Memory implementation
 */
class mem_ota : public IHal_Mem_Ota
{
private:
    /** VARIABLES *************************************************************/
    esp_ota_handle_t       _updateHandle;    // Handle for the OTA update process
    const esp_partition_t* _updatePartition; // Pointer to the partition where the new firmware will be written
    bool                   _isInitialized;   // Flag indicating whether the OTA memory interface has been initialized
    bool                   _isOngoing;       // Flag indicating whether an OTA update session is currently in progress
    bool                   _headerValidated; // Flag indicating whether the incoming firmware image header has been validated

    /** PRIVATE METHODS *******************************************************/
    sys_error_t validateIncomingImageHeader(const uint8_t* data, size_t length);

public:
    mem_ota();
    ~mem_ota();

    // Disable copy mechanics to prevent duplicate handles to the same OTA session
    mem_ota(const mem_ota&)            = delete;
    mem_ota& operator=(const mem_ota&) = delete;

    // Disable move mechanics unless specifically needed
    mem_ota(mem_ota&&)            = delete;
    mem_ota& operator=(mem_ota&&) = delete;

    /** INTERFACE METHODS *****************************************************/
    virtual sys_error_t init(void* params = nullptr) override;
    virtual sys_error_t deInit() override;
    virtual sys_error_t begin(size_t imageSize) override;
    virtual sys_error_t write(const uint8_t* data, size_t length) override;
    virtual sys_error_t end() override;
    virtual sys_error_t abort() override;
    virtual sys_error_t setBootPartition() override;
    virtual sys_error_t markAppValid() override;
    virtual sys_error_t markAppInvalid() override;

public:
    /** USER METHODS *******************************************************/
    void resetInternalState();
};

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** FUNCTIONS *****************************************************************/

#endif // MEM_OTA_HPP
