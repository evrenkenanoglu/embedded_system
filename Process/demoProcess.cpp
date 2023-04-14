/** @file       demoprocess.c
 *  @brief      Demo Process CPP Template
 *  @copyright  (c) 2021- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       03/10/2021
 */
#define FILE_DEMOPROCESS_C

/** INCLUDES ******************************************************************/
#include "demoProcess.h"

using namespace std;
const char *processTag = "Demo Process 1";
const char *tagTask1 = "Task1";
/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/
#define TASK_DEMO_DELAY_INTERVAL 1000 // ms
#define TASK_DEMO_1_QUEUE_SIZE 5
/** VARIABLES *****************************************************************/

/** LOCAL FUNCTION DECLARATIONS ***********************************************/
static void Delay(void);
static void taskDemo1(void *pvParameters);
TaskHandle_t xHandle;

/** INTERFACE FUNCTION DEFINITIONS ********************************************/

uint32_t DemoProcess::start()
{
    ESP_LOGI(processTag, " Process Started!");
    demoParams_t *params = (demoParams_t *)this->_parameters;
    const demoConsts_t *consts = (demoConsts_t *)this->_constants;



    return EXIT_SUCCESS;
}

uint32_t DemoProcess::suspend()
{
    return EXIT_SUCCESS;
}
uint32_t DemoProcess::resume()
{
    return EXIT_SUCCESS;
}
uint32_t DemoProcess::terminate()
{
    return EXIT_SUCCESS;
}

/** LOCAL FUNCTION DEFINITIONS ************************************************/
