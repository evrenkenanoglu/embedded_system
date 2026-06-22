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

#include "esp_ota_ops.h"
#include "HAL/IHAL/IHal_Mem_Ota.h"

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
    esp_ota_handle_t          _updateHandle;
    const esp_partition_t*    _updatePartition;
    bool                      _isInitialized;
    bool                      _isOngoing;
    bool                      _headerValidated;

    /** PRIVATE METHODS *******************************************************/
    sys_error_t validateIncomingImageHeader(const uint8_t* data, size_t length);

public:
    mem_ota();
    ~mem_ota();

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
};

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** FUNCTIONS *****************************************************************/

#endif // MEM_OTA_HPP
