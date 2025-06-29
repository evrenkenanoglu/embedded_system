import sys
from datetime import date
import templates.Generic as GenericTemplate

# Get the file name from the user
filename = input("Enter the file name: ").strip()

# Give information to the user about the file types
print("The system file types are:")
print("----------------------------------------")
print(" HAL - Hardware Abstraction Layer Types:")
print("     IO  - Input/Output")
print("     COM - Communication")
print("     MEM - Memory")
print("     CPX - Complex")
print("----------------------------------------")
print(" PAL - Platform Abstraction Layer Types:")
print("     SERV - Platform Service")
print("----------------------------------------")
print(" APP - Application Layer Types:")
print("     PROC - Process")
print("----------------------------------------")

# Get the system file type from the user
system_file_type = input(
    "Enter the system type (IO, COM, MEM, CPX, PROC, SERV): "
).upper()

header_template, source_template = GenericTemplate.get_function_implementations(
    system_file_type
)

if system_file_type != "":
    system_file_type.lower()
    if (
        system_file_type == "IO"
        or system_file_type == "COM"
        or system_file_type == "MEM"
        or system_file_type == "CPX"
    ):
        filename = system_file_type.lower() + "_" + filename.lower()
    else:
        filename = system_file_type.capitalize() + "_" + filename.capitalize()

# Get the class name from the user
classname = filename

# Get the brief description from the user
brief = input("Enter a brief description: ")

abstract_header_file = ""
abstract_classname = ""

# Process Definitions
iprocess_header_file = ""
iprocess_classname = ""
iprocess_start_definition = ""
iprocess_stop_definition = ""
iprocess_pause_definition = ""
iprocess_resume_definition = ""

# HAL Definitions
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

start_definition = ""
cpx_get_definition = ""
cpx_set_definition = ""
stop_definition = ""

# PAL Definitions

pal_serv_header_file = ""
pal_serv_classname = ""
pal_serv_init_definition = ""
pal_serv_start_definition = ""
pal_serv_stop_definition = ""
pal_serv_restart_definition = ""

if system_file_type == "IO":
    ihal_header_file = "IHal.h"
    ihal_classname = "IHAL_IO"
    get_definition = "void " + classname + "::get(void* data) override;\n"
    set_definition = "sys_error_t " + classname + "::set(void* data) override;\n"

elif system_file_type == "COM":
    ihal_header_file = "IHal.h"
    ihal_classname = "IHAL_COM"
    connect_definition = "sys_error_t " + classname + "::connect() override;\n"
    disconnect_definition = "void " + classname + "::disconnect() override;\n"
    sendData_definition = (
        "sys_error_t "
        + classname
        + "::sendData(const uint8_t* data, size_t length) override;\n"
    )
    receiveData_definition = (
        "sys_error_t "
        + classname
        + "::receiveData(uint8_t* data, size_t maxLength, size_t& receivedLength) override;\n"
    )

elif system_file_type == "MEM":
    ihal_header_file = "IHal.h"
    ihal_classname = "IHAL_MEM"
    initialize_definition = "sys_error_t " + classname + "::init() override;\n"
    readData_definition = (
        "sys_error_t "
        + classname
        + "::readData(const void *addressOrKey, uint8_t* data, size_t length) override;\n"
    )
    writeData_definition = (
        "sys_error_t "
        + classname
        + "::writeData(const void* addressOrKey, const uint8_t* data, size_t length) override;\n"
    )
    erase_definition = (
        "sys_error_t " + classname + "::erase(const void* addressOrKey) override;\n"
    )
    getSize_definition = (
        "sys_error_t " + classname + "::getSize(uint32_t *size) override;\n"
    )

elif system_file_type == "CPX":
    ihal_header_file = "IHal.h"
    ihal_classname = "IHAL_CPX"
    start_definition = "sys_error_t " + classname + "::start() override;\n"
    cpx_get_definition = "void* " + classname + "::get() override;\n"
    cpx_set_definition = "sys_error_t " + classname + "::set(void* data) override;\n"
    stop_definition = "sys_error_t " + classname + "::stop() override;\n"

elif system_file_type == "PROC":
    iprocess_header_file = "Process/Process.hpp"
    iprocess_classname = "Process"
    iprocess_start_definition = "sys_error_t " + classname + "::start() override;\n"
    iprocess_stop_definition = "sys_error_t " + classname + "::stop() override;\n"
    iprocess_pause_definition = "sys_error_t " + classname + "::pause() override;\n"
    iprocess_resume_definition = "sys_error_t " + classname + "::resume() override;\n"

elif system_file_type == "SERV":
    pal_serv_header_file = "Pal/Pal.hpp"
    pal_serv_classname = "PAL_Service"
    pal_serv_init_definition = "sys_error_t " + classname + "::init() override;\n"
    pal_serv_start_definition = "sys_error_t " + classname + "::start() override;\n"
    pal_serv_stop_definition = "sys_error_t " + classname + "::stop() override;\n"
    pal_serv_restart_definition = "sys_error_t " + classname + "::restart() override;\n"

else:
    print("Generic C++ Files Created!")

# Generate the file contents using the templates and the user input

def create_header_content(
    filename, classname, brief, abstract_header_file, abstract_classname, **kwargs
):

    # For each key in the dictionary kwargs, remove the classname from the value
    for key, value in kwargs.items():
        value = value.replace(classname + "::", "")
        kwargs[key] = value

    header_content = header_template.format(
        filename=filename,
        filename_upper=filename.upper(),
        classname=classname,
        brief=brief,
        abstract_header_file=abstract_header_file,
        abstract_classname=abstract_classname,
        **kwargs  # Dynamically include the rest of the variables
    )
    return header_content


def create_source_content(filename, classname, **kwargs):

    # For each key in the dictionary kwargs, remove the override keyword from the value
    for key, value in kwargs.items():
        value = value.replace("override", "")
        kwargs[key] = value

    source_content = source_template.format(
        filename=filename,
        classname=classname,
        **kwargs  # Dynamically include the rest of the variables
    )
    return source_content


header_content = ""
source_content = ""



if system_file_type == "IO":
    header_content = create_header_content(
        filename,
        classname,
        brief,
        ihal_header_file,
        ihal_classname,
        get_definition=get_definition,
        set_definition=set_definition,
    )
    source_content = create_source_content(
        filename,
        classname,
        get_definition=get_definition,
        set_definition=set_definition,
    )
elif system_file_type == "COM":
    header_content = create_header_content(
        filename,
        classname,
        brief,
        ihal_header_file,
        ihal_classname,
        connect_definition=connect_definition,
        sendData_definition=sendData_definition,
        receiveData_definition=receiveData_definition,
        disconnect_definition=disconnect_definition,
    )
    source_content = create_source_content(
        filename,
        classname,
        connect_definition=connect_definition,
        sendData_definition=sendData_definition,
        receiveData_definition=receiveData_definition,
        disconnect_definition=disconnect_definition,
    )

elif system_file_type == "MEM":
    header_content = create_header_content(
        filename,
        classname,
        brief,
        ihal_header_file,
        ihal_classname,
        initialize_definition=initialize_definition,
        readData_definition=readData_definition,
        writeData_definition=writeData_definition,
        erase_definition=erase_definition,
        getSize_definition=getSize_definition,
    )
    source_content = create_source_content(
        filename,
        classname,
        initialize_definition=initialize_definition,
        readData_definition=readData_definition,
        writeData_definition=writeData_definition,
        erase_definition=erase_definition,
        getSize_definition=getSize_definition,
    )
elif system_file_type == "CPX":
    header_content = create_header_content(
        filename,
        classname,
        brief,
        ihal_header_file,
        ihal_classname,
        start_definition=start_definition,
        cpx_get_definition=cpx_get_definition,
        cpx_set_definition=cpx_set_definition,
        stop_definition=stop_definition,
    )
    source_content = create_source_content(
        filename,
        classname,
        start_definition=start_definition,
        cpx_get_definition=cpx_get_definition,
        cpx_set_definition=cpx_set_definition,
        stop_definition=stop_definition,
    )

elif system_file_type == "PROC":
    header_content = create_header_content(
        filename,
        classname,
        brief,
        iprocess_header_file,
        iprocess_classname,
        iprocess_header_file=iprocess_header_file,
        iprocess_classname=iprocess_classname,
        iprocess_start_definition=iprocess_start_definition,
        iprocess_stop_definition=iprocess_stop_definition,
        iprocess_pause_definition=iprocess_pause_definition,
        iprocess_resume_definition=iprocess_resume_definition,
    )
    source_content = create_source_content(
        filename,
        classname,
        iprocess_start_definition=iprocess_start_definition,
        iprocess_stop_definition=iprocess_stop_definition,
        iprocess_pause_definition=iprocess_pause_definition,
        iprocess_resume_definition=iprocess_resume_definition,
    )
elif system_file_type == "SERV":
    header_content = create_header_content(
        filename,
        classname,
        brief,
        pal_serv_header_file,
        pal_serv_classname,
        pal_serv_header_file=pal_serv_header_file,
        pal_serv_classname=pal_serv_classname,
        pal_serv_init_definition=pal_serv_init_definition,
        pal_serv_start_definition=pal_serv_start_definition,
        pal_serv_stop_definition=pal_serv_stop_definition,
        pal_serv_restart_definition=pal_serv_restart_definition,
    )
    source_content = create_source_content(
        filename,
        classname,
        pal_serv_init_definition=pal_serv_init_definition,
        pal_serv_start_definition=pal_serv_start_definition,
        pal_serv_stop_definition=pal_serv_stop_definition,
        pal_serv_restart_definition=pal_serv_restart_definition,
    )


# header_content = header_template.format(
#     filename=filename,
#     filename_upper=filename.upper(),
#     classname=classname,
#     brief=brief,
#     abstract_header_file=(
#         ihal_header_file if ihal_header_file != "" else if iprocess_header_file != "" else

#     ),
#     abstract_classname=ihal_classname if ihal_classname != "" else iprocess_classname,

#     get_definition=get_definition.replace(classname + "::", ""),
#     set_definition=set_definition.replace(classname + "::", ""),

#     connect_definition=connect_definition.replace(classname + "::", ""),
#     sendData_definition=sendData_definition.replace(classname + "::", ""),
#     receiveData_definition=receiveData_definition.replace(classname + "::", ""),
#     disconnect_definition=disconnect_definition.replace(classname + "::", ""),

#     initialize_definition=initialize_definition.replace(classname + "::", ""),
#     readData_definition=readData_definition.replace(classname + "::", ""),
#     writeData_definition=writeData_definition.replace(classname + "::", ""),
#     erase_definition=erase_definition.replace(classname + "::", ""),
#     getSize_definition=getSize_definition.replace(classname + "::", ""),

#     start_definition=start_definition.replace(classname + "::", ""),
#     cpx_get_definition=cpx_get_definition.replace(classname + "::", ""),
#     cpx_set_definition=cpx_set_definition.replace(classname + "::", ""),
#     stop_definition=stop_definition.replace(classname + "::", ""),

#     iprocess_header_file=iprocess_header_file,
#     iprocess_classname=iprocess_classname,
#     iprocess_start_definition=iprocess_start_definition.replace(classname + "::", ""),
#     iprocess_stop_definition=iprocess_stop_definition.replace(classname + "::", ""),
#     iprocess_pause_definition=iprocess_pause_definition.replace(classname + "::", ""),
#     iprocess_resume_definition=iprocess_resume_definition.replace(classname + "::", ""),

#     pal_serv_header_file=pal_serv_header_file,
#     pal_serv_classname=pal_serv_classname,
#     pal_serv_init_definition=pal_serv_init_definition.replace(
#         classname + "::", ""
#     ),
#     pal_serv_start_definition=pal_serv_start_definition.replace(
#         classname + "::", ""
#     ),
#     pal_serv_stop_definition=pal_serv_stop_definition.replace(
#         classname + "::", ""
#     ),
#     pal_serv_restart_definition=pal_serv_restart_definition.replace(
#         classname + "::", ""
#     ),
# )

# source_content = source_template.format(
#     filename=filename,
#     classname=classname,
#     get_definition=get_definition.replace("override", ""),
#     set_definition=set_definition.replace("override", ""),
#     connect_definition=connect_definition.replace("override", ""),
#     sendData_definition=sendData_definition.replace("override", ""),
#     receiveData_definition=receiveData_definition.replace("override", ""),
#     disconnect_definition=disconnect_definition.replace("override", ""),
#     initialize_definition=initialize_definition.replace("override", ""),
#     readData_definition=readData_definition.replace("override", ""),
#     writeData_definition=writeData_definition.replace("override", ""),
#     erase_definition=erase_definition.replace("override", ""),
#     getSize_definition=getSize_definition.replace("override", ""),
#     start_definition=start_definition.replace("override", ""),
#     cpx_get_definition=cpx_get_definition.replace("override", ""),
#     cpx_set_definition=cpx_set_definition.replace("override", ""),
#     stop_definition=stop_definition.replace("override", ""),
#     iprocess_start_definition=iprocess_start_definition.replace("override", ""),
#     iprocess_stop_definition=iprocess_stop_definition.replace("override", ""),
#     iprocess_pause_definition=iprocess_pause_definition.replace("override", ""),
#     iprocess_resume_definition=iprocess_resume_definition.replace("override", ""),
#     pal_serv_init_definition=pal_serv_init_definition.replace("override", ""),
#     pal_serv_start_definition=pal_serv_start_definition.replace("override", ""),
#     pal_serv_stop_definition=pal_serv_stop_definition.replace("override", ""),
#     pal_serv_restart_definition=pal_serv_restart_definition.replace(
#         "override", ""
#     ),
# )

# Write the generated file contents to disk
with open(filename + ".hpp", "w") as header_file:
    header_file.write(header_content)
    print("Created " + filename + ".hpp")

with open(filename + ".cpp", "w") as source_file:
    source_file.write(source_content)
    print("Created " + filename + ".cpp")

