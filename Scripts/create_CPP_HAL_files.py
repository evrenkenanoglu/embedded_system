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

#include "{ihal_header_file}"

class {classname} : public {ihal_classname} 
{{
private:
    // private members

public:
    {classname}();
    ~{classname}();

    {get_definition}
    {set_definition}
    {connect_definition}
    {sendData_definition}
    {receiveData_definition}
    {disconnect_definition}
    {initialize_definition}
    {readData_definition}
    {writeData_definition}
    {erase_definition}
    {getSize_definition}
    {start_definition}
    {cpx_get_definition}
    {cpx_set_definition}
    {stop_definition}

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

#include "IHal.hpp"

{classname}::{classname}() 
{{
    // constructor implementation
}}

{classname}::~{classname}() 
{{
    // destructor implementation
}}

{get_definition}
{set_definition}
{connect_definition}
{sendData_definition}
{receiveData_definition}
{disconnect_definition}
{initialize_definition}
{readData_definition}
{writeData_definition}
{erase_definition}
{getSize_definition}
{start_definition}
{cpx_get_definition}
{cpx_set_definition}
{stop_definition}
"""

# Get the file name from the user
filename = input("Enter the file name: ")

# Get the HAL type from the user
hal_type = input("Enter the HAL type (IO, COM, MEM, CPX): ").upper()

if  hal_type != "":
    filename = hal_type.lower() + "_" + filename

# Get the class name from the user
classname = filename

# Get the brief description from the user
brief = input("Enter a brief description: ")

ihal_header_file = ""
ihal_classname = ""

# IO Definitions
get_definition = ""
set_definition = ""

# COM Definintions
connect_definition = ""
sendData_definition = ""
receiveData_definition = ""
disconnect_definition = ""

# MEM Definitions

initialize_definition = ""
readData_definition = ""
writeData_definition = ""
erase_definition = ""
getSize_definition = ""

# CPX Definitions

start_definition= ""
cpx_get_definition= ""
cpx_set_definition= ""
stop_definition= ""

if hal_type == "IO":
    ihal_header_file    = "ihal_io.h"
    ihal_classname      = "IHAL_IO"
    get_definition      = "void "    + classname  + "::get(void* data) override;\n"
    set_definition      = "error_t " + classname  + "::set(void* data) override;\n"

elif hal_type == "COM":
    ihal_header_file        = "ihal_com.h"
    ihal_classname          = "IHAL_COM"
    connect_definition      = "error_t "    + classname + "::connect() override;\n"
    disconnect_definition   = "void "       + classname + "::disconnect() override;\n"
    sendData_definition     = "error_t "    + classname + "::sendData(const uint8_t* data, size_t length) override;\n"
    receiveData_definition  = "error_t "    + classname + "::receiveData(uint8_t* data, size_t maxLength, size_t& receivedLength) override;\n"

elif hal_type == "MEM":
    ihal_header_file        = "ihal_mem.h"
    ihal_classname          = "IHAL_MEM"
    initialize_definition   = "bool "   + classname + "::initialize() override;\n"
    readData_definition     = "bool "   + classname + "::readData(uint32_t address, uint8_t* data, size_t length) override;\n"
    writeData_definition    = "bool "   + classname + "::writeData(uint32_t address, const uint8_t* data, size_t length) override;\n"
    erase_definition        = "bool "   + classname + "::erase() override;\n"
    getSize_definition      = "size_t " + classname + "::getSize() override;\n"

elif hal_type == "CPX":
    ihal_header_file    = "ihal_cpx.h"
    ihal_classname      = "IHAL_CPX"
    start_definition    = "error_t "    + classname + "::start() override;\n"
    cpx_get_definition  = "void* "      + classname + "::get() override;\n"
    cpx_set_definition  = "error_t "    + classname + "::set(void* data) override;\n"
    stop_definition     = "error_t "    + classname + "::stop() override;\n"

else:
    ihal_header_file = ""
    ihal_classname = ""
    print("Generic C++ Files Created!")

# Generate the file contents using the templates and the user input
header_content = header_template.format(
    filename=filename,
    filename_upper=filename.upper(),
    classname=classname,
    brief=brief,
    ihal_header_file=ihal_header_file,
    ihal_classname=ihal_classname,
    get_definition=get_definition.replace(classname + "::", ""),
    set_definition=set_definition.replace(classname + "::", ""),
    connect_definition = connect_definition.replace(classname + "::", ""),
    sendData_definition = sendData_definition.replace(classname + "::", ""),
    receiveData_definition = receiveData_definition.replace(classname + "::", ""),
    disconnect_definition = disconnect_definition.replace(classname + "::", ""),
    initialize_definition = initialize_definition.replace(classname + "::", ""),
    readData_definition = readData_definition.replace(classname + "::", ""),
    writeData_definition = writeData_definition.replace(classname + "::", ""),
    erase_definition = erase_definition.replace(classname + "::", ""),
    getSize_definition = getSize_definition.replace(classname + "::", ""),
    start_definition = start_definition.replace(classname + "::", ""),
    cpx_get_definition = cpx_get_definition.replace(classname + "::", ""),
    cpx_set_definition = cpx_set_definition.replace(classname + "::", ""),
    stop_definition = stop_definition.replace(classname + "::", ""),
)
source_content = source_template.format(
    filename=filename,
    classname=classname,
    get_definition=get_definition.replace("override", ""),
    set_definition=set_definition.replace("override", ""),
    connect_definition = connect_definition.replace("override", ""),
    sendData_definition = sendData_definition.replace("override", ""),
    receiveData_definition = receiveData_definition.replace("override", ""),
    disconnect_definition = disconnect_definition.replace("override", ""),
    initialize_definition = initialize_definition.replace("override", ""),
    readData_definition = readData_definition.replace("override", ""),
    writeData_definition = writeData_definition.replace("override", ""),
    erase_definition = erase_definition.replace("override", ""),
    getSize_definition = getSize_definition.replace("override", ""),
    start_definition = start_definition.replace("override", ""),
    cpx_get_definition = cpx_get_definition.replace("override", ""),
    cpx_set_definition = cpx_set_definition.replace("override", ""),
    stop_definition = stop_definition.replace("override", ""),
)

# Write the generated file contents to disk
with open(filename + ".hpp", "w") as header_file:
    header_file.write(header_content)
    print("Created " + filename + ".hpp")

with open(filename + ".cpp", "w") as source_file:
    source_file.write(source_content)
    print("Created " + filename + ".cpp")
