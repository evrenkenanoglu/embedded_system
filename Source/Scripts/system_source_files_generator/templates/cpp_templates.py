def get_header_template():
    return """/** @file       {filename}.hpp
 *  @brief      {brief}
 *  @copyright  (c) {copyright_year} {author} - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of {author}.
 *  @author     {author}
 *  @date       {current_date}
 */
#ifndef {header_guard}
#define {header_guard}

#include "HAL/IHAL/{interface_header}"

/** INCLUDES ******************************************************************/

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

/**
 * @class {classname}
 * @brief {brief}
 */
class {classname} : public {interface_class}
{{
private:
    /** VARIABLES *************************************************************/
    
public:
    {classname}();
    ~{classname}();
    
    /** INTERFACE METHODS *****************************************************/
    {methods_section}
public:
    /** USER METHODS *******************************************************/
}};

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** FUNCTIONS *****************************************************************/

#endif // {header_guard}
"""


def get_source_template():
    return """/** @file       {filename}.cpp
 *  @brief      {brief}
 *  @copyright  (c) {copyright_year} {author} - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of {author}.
 *  @author     {author}
 *  @date       {current_date}
 */

/** INCLUDES ******************************************************************/
#include "{filename}.hpp"

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** LOCAL FUNCTIONS ***********************************************************/

/** FUNCTIONS *****************************************************************/

{classname}::{classname}()
{{
    // TODO: Initialize member variables
}}

{classname}::~{classname}()
{{
    // TODO: Cleanup if needed
}}
{methods_section}
"""
