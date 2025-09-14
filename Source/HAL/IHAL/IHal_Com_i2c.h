#ifndef FILE_IHAL_COM_I2C_H
#define FILE_IHAL_COM_I2C_H

#include "IHal.h"

/** INCLUDES ******************************************************************/

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/
// Platform-independent I2C mode enumeration
typedef enum
{
    HAL_I2C_MODE_MASTER = 0,
    HAL_I2C_MODE_SLAVE,
    HAL_I2C_MODE_MASTER_SLAVE
} hal_i2c_mode_t;

// Platform-independent I2C speed presets
typedef enum
{
    HAL_I2C_SPEED_STANDARD  = 100000,  // 100 kHz
    HAL_I2C_SPEED_FAST      = 400000,  // 400 kHz
    HAL_I2C_SPEED_FAST_PLUS = 1000000, // 1 MHz
    HAL_I2C_SPEED_HIGH      = 3400000  // 3.4 MHz
} hal_i2c_speed_preset_t;

// Platform-independent GPIO configuration
typedef struct
{
    uint32_t pin_number;    // GPIO pin number (platform-specific interpretation)
    bool     pullup_enable; // Enable internal pull-up
    bool     open_drain;    // Configure as open-drain (useful for I2C)
} hal_i2c_gpio_config_t;

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** FUNCTIONS *****************************************************************/

// Updated platform-independent I2C configuration
typedef struct
{
    uint32_t              clk_speed;  // I2C clock speed in Hz
    hal_i2c_mode_t        mode;       // I2C operating mode
    uint8_t               port_id;    // Logical port identifier
    hal_i2c_gpio_config_t sda_config; // SDA pin configuration
    hal_i2c_gpio_config_t scl_config; // SCL pin configuration
    uint32_t              timeout_ms; // Transaction timeout in milliseconds

    // Platform-specific extensions (optional)
    const void* platform_config; // Pointer to platform-specific configuration
} hal_com_i2c_config_t;

/**
 * @class IHAL_COM_I2C
 * @brief Interface for Hardware Abstraction Layer (HAL) I2C communication operations.
 */
class IHAL_COM_I2C : public IHAL_COM
{
};

#endif // FILE_IHAL_COM_I2C_H