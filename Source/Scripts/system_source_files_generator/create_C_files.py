import sys
from datetime import date

# Define the header and source file templates
header_template = """\
/** @file       
 *  @brief      {brief}
 *  @copyright  (c) {year}- {author} - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of {author}.
 *  @author   
 *  @date       {date}
 */

#ifndef {filename_upper}_H
#define {filename_upper}_H

/** INCLUDES ******************************************************************/

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

#ifndef {filename_upper}_C
#define INTERFACE extern
#else
#define INTERFACE
#endif
/** VARIABLES *****************************************************************/

/** LOCAL FUNCTION DECLARATIONS ***********************************************/

/** INTERFACE FUNCTION DECLARATIONS ******************************************/

/** LOCAL FUNCTION DEFINITIONS ************************************************/


#undef INTERFACE // Should not let this roam free

#endif /* {filename_upper}_H */
"""

source_template = """\
/** @file       
 *  @brief      {brief}
 *  @copyright  (c) {year}- {author} - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of {author}.
 *  @author   
 *  @date       {date}
 */

/** INCLUDES ******************************************************************/

#include "{filename}.h"

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** LOCAL FUNCTION DECLARATIONS ***********************************************/

/** INTERFACE FUNCTION DEFINITIONS ********************************************/

/** LOCAL FUNCTION DEFINITIONS ************************************************/
"""

# Ask user for inputs
filename = input("Enter the filename: ")
brief = input("Enter the brief description: ")

# Get the current date and year
today = date.today()
current_year = today.strftime("%Y")

# Generate the file contents using the templates and the user inputs
header_content = header_template.format(filename=filename, filename_upper=filename.upper(), brief=brief, year=current_year, author="Evren Kenanoglu", date=today.strftime("%d %B %Y"))
source_content = source_template.format(filename=filename, brief=brief, year=current_year, author="Evren Kenanoglu", date=today.strftime("%d %B %Y"))

# Write the generated file contents to disk
with open(filename + ".h", "w") as header_file:
    header_file.write(header_content)
    print("Created " + filename + ".h")

with open(filename + ".c", "w") as source_file:
    source_file.write(source_content)
    print("Created " + filename + ".c")
