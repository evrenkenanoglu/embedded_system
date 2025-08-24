import sys
from datetime import date

# Define the header and source file templates
header_template = """\
/**
 * @file {filename}.hpp
 * @brief Header file for {filename}
 *
 * This file contains declarations for the {filename} class and related data types and functions.
 */

#ifndef {filename_upper}_HPP
#define {filename_upper}_HPP

class {classname} {{
private:
    // private members

public:
    {classname}();
    ~{classname}();

}};

#endif /* {filename_upper}_HPP */
"""

source_template = """\
/**
 * @file {filename}.cpp
 * @brief Source file for {filename}
 *
 * This file contains definitions for the {filename} class and related data types and functions.
 */

#include "{filename}.hpp"

{classname}::{classname}() {{
    // constructor implementation
}}

{classname}::~{classname}() {{
    // destructor implementation
}}
"""

# Get the file name from the user
filename = input("Enter the file name: ")

# Get the class name from the user
classname = input("Enter the class name: ")

# Get the brief description from the user
brief = input("Enter a brief description: ")

# Get the author name from the user
author = input("Enter your name: ")

# Get the current date and year
today = date.today()
current_year = today.strftime("%Y")

# Generate the file contents using the templates and the user input
header_content = header_template.format(filename=filename, filename_upper=filename.upper(), classname=classname, brief=brief, author=author)
source_content = source_template.format(filename=filename, classname=classname)

# Write the generated file contents to disk
with open(filename + ".hpp", "w") as header_file:
    header_file.write(header_content)
    print("Created " + filename + ".hpp")

with open(filename + ".cpp", "w") as source_file:
    source_file.write(source_content)
    print("Created " + filename + ".cpp")
