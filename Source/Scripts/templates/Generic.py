# import path
import os
import importlib.util


# create a function to get the templates
def get_function_implementations(sys_type):

    # Get the this script's directory absolute path
    script_dir = os.path.dirname(os.path.abspath(__file__))

    header_template = ""
    source_template = ""
    module_path = ""
    if sys_type == "IO":
        module_path = os.path.join(script_dir, "Hal_Io.py")

    elif sys_type == "COM":
        module_path = os.path.join(script_dir, "Hal_Com.py")
    elif sys_type == "MEM":
        module_path = os.path.join(script_dir, "Hal_Mem.py")
    elif sys_type == "CPX":
        module_path = os.path.join(script_dir, "Hal_Cpx.py")
    elif sys_type == "PROC":
        module_path = os.path.join(script_dir, "Proc.py")
    elif sys_type == "SERV":
        module_path = os.path.join(script_dir, "Pal_Serv.py")
    else:
        module_path = os.path.join(script_dir, "Generic.py")

    function_implementations = import_module_from_path(
        module_path, "function_implementations"
    )

    header_template = (
        header_template_begin + function_implementations + header_template_end
    )
    source_template = (
        source_template_begin + function_implementations + source_template_end
    )

    return header_template, source_template


header_template_begin = """\
/**
 * @file {filename}.hpp
 * @brief Header file for {filename}
 *
 * This file contains declarations for the {filename} class and related data types and functions.
 */

#ifndef {filename_upper}_HPP
#define {filename_upper}_HPP

#include "{abstract_header_file}"

class {classname} : public {abstract_classname} 
{{
private:
    // private members

public:
    {classname}();
    ~{classname}();

"""

header_template_end = """\
}};
#endif /* {filename_upper}_HPP */
"""


source_template_begin = """\
/**
 * @file {filename}.cpp
 * @brief Source file for {filename}
 *
 * This file contains definitions for the {filename} class and related data types and functions.
 */

#include "{classname}.hpp"

{classname}::{classname}() 
{{
    // constructor implementation
}}

{classname}::~{classname}() 
{{
    // destructor implementation
}}

"""
source_template_end = ""


def import_module_from_path(module_path, function_name):
    spec = importlib.util.spec_from_file_location("module.name", module_path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return getattr(module, function_name)
