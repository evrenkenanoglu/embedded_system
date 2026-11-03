/** @file       hil_test_io_gpio.cpp
 *  @brief      Generic HIL loopback test for two GPIO instances (drive and sense pins wired together).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 */

/** INCLUDES ******************************************************************/
#include "hil_test_io_gpio.hpp"

/** FUNCTIONS *****************************************************************/

/**
 * @brief Reads the sense pin and checks it matches the expected level.
 */
static sys_error_t expectSenseLevel(IHAL_IO_GPIO& senseGpio, hal_gpio_level_t expected)
{
    hal_gpio_level_t  level = hal_gpio_level_t::LOW;
    const sys_error_t err   = senseGpio.getLevel(level);
    if (err != ERROR_SUCCESS)
    {
        return err;
    }
    if (level != expected)
    {
        return ERROR_FAIL;
    }
    return ERROR_SUCCESS;
}

/**
 * @brief Runs the drive/sense sequence: explicit LOW and HIGH, then two toggles.
 */
static sys_error_t runLoopbackSequence(IHAL_IO_GPIO& driveGpio, IHAL_IO_GPIO& senseGpio)
{
    sys_error_t err = driveGpio.init();
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    err = senseGpio.init();
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    err = driveGpio.setDirection(hal_gpio_direction_t::OUTPUT);
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    err = senseGpio.setDirection(hal_gpio_direction_t::INPUT);
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    err = driveGpio.setLevel(hal_gpio_level_t::LOW);
    if (err != ERROR_SUCCESS)
    {
        return err;
    }
    err = expectSenseLevel(senseGpio, hal_gpio_level_t::LOW);
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    err = driveGpio.setLevel(hal_gpio_level_t::HIGH);
    if (err != ERROR_SUCCESS)
    {
        return err;
    }
    err = expectSenseLevel(senseGpio, hal_gpio_level_t::HIGH);
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    err = driveGpio.toggleLevel();
    if (err != ERROR_SUCCESS)
    {
        return err;
    }
    err = expectSenseLevel(senseGpio, hal_gpio_level_t::LOW);
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    err = driveGpio.toggleLevel();
    if (err != ERROR_SUCCESS)
    {
        return err;
    }
    return expectSenseLevel(senseGpio, hal_gpio_level_t::HIGH);
}

sys_error_t hil_test_io_gpio_contract(IHAL_IO_GPIO& driveGpio, IHAL_IO_GPIO& senseGpio)
{
    const sys_error_t testErr = runLoopbackSequence(driveGpio, senseGpio);
    const sys_error_t safeErr = driveGpio.setLevel(hal_gpio_level_t::LOW);
    if (testErr != ERROR_SUCCESS)
    {
        return testErr;
    }
    return safeErr;
}
