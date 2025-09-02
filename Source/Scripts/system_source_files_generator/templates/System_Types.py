SYSTEM_TYPES = {
    ##############################################################################################
    "IO": {
        "layer": "HAL",
        "description": "Input/Output",
        "interface": {"header": "IHal.h", "class": "IHAL_IO"},
        "methods": {
            "init": {
                "return_type": "sys_error_t",
                "parameters": "void* params = nullptr",
                "override": True,
            },
            "deInit": {
                "return_type": "sys_error_t",
                "parameters": "",
                "override": True,
            },
            "get": {
                "return_type": "void",
                "parameters": "void* data",
                "override": True,
            },
            "set": {
                "return_type": "sys_error_t",
                "parameters": "void* data",
                "override": True,
            },
        },
    },
    ##############################################################################################
    "COM": {
        "layer": "HAL",
        "description": "Communication",
        "interface": {"header": "IHal.h", "class": "IHAL_COM"},
        "methods": {
            "init": {
                "return_type": "sys_error_t",
                "parameters": "void* params = nullptr",
                "override": True,
            },
            "deInit": {
                "return_type": "sys_error_t",
                "parameters": "",
                "override": True,
            },
            "connect": {
                "return_type": "sys_error_t",
                "parameters": "",
                "override": True,
            },
            "disconnect": {
                "return_type": "sys_error_t",
                "parameters": "",
                "override": True,
            },
            "sendData": {
                "return_type": "sys_error_t",
                "parameters": "const void* deviceAddress, const uint8_t* data, size_t length",
                "override": True,
            },
            "receiveData": {
                "return_type": "sys_error_t",
                "parameters": "const void* deviceAddress, uint8_t* data, size_t maxLength, size_t& receivedLength",
                "override": True,
            },
            "writeRead": {
                "return_type": "sys_error_t",
                "parameters": "const void* deviceAddress, const uint8_t* writeData, size_t writeSize, uint8_t* readData, size_t readSize",
                "override": True,
            },
        },
    },
    ##############################################################################################
    "MEM": {
        "layer": "HAL",
        "description": "Memory",
        "interface": {"header": "IHal.h", "class": "IHAL_MEM"},
        "methods": {
            "init": {
                "return_type": "sys_error_t",
                "parameters": "void* params = nullptr",
                "override": True,
            },
            "deInit": {
                "return_type": "sys_error_t",
                "parameters": "",
                "override": True,
            },
            "readData": {
                "return_type": "sys_error_t",
                "parameters": "const void* addressOrKey, uint8_t* data, size_t length",
                "override": True,
            },
            "writeData": {
                "return_type": "sys_error_t",
                "parameters": "const void* addressOrKey, const uint8_t* data, size_t length",
                "override": True,
            },
            "erase": {
                "return_type": "sys_error_t",
                "parameters": "const void* addressOrKey",
                "override": True,
            },
            "getSize": {
                "return_type": "sys_error_t",
                "parameters": "uint32_t* size",
                "override": True,
            },
        },
    },
    ##############################################################################################
    "CPX": {
        "layer": "HAL",
        "description": "Complex",
        "interface": {"header": "IHal.h", "class": "IHAL_CPX"},
        "methods": {
            "init": {
                "return_type": "sys_error_t",
                "parameters": "void* params = nullptr",
                "override": True,
            },
            "deInit": {
                "return_type": "sys_error_t",
                "parameters": "",
                "override": True,
            },
            "start": {"return_type": "sys_error_t", "parameters": "", "override": True},
            "get": {
                "return_type": "sys_error_t",
                "parameters": "void* data",
                "override": True,
            },
            "set": {
                "return_type": "sys_error_t",
                "parameters": "void* data",
                "override": True,
            },
            "stop": {"return_type": "sys_error_t", "parameters": "", "override": True},
        },
    },
    ##############################################################################################
    "PROC": {
        "layer": "APP",
        "description": "Process",
        "interface": {"header": "Process/Process.hpp", "class": "Process"},
        "methods": {
            "start": {"return_type": "sys_error_t", "parameters": "", "override": True},
            "stop": {"return_type": "sys_error_t", "parameters": "", "override": True},
            "pause": {"return_type": "sys_error_t", "parameters": "", "override": True},
            "resume": {
                "return_type": "sys_error_t",
                "parameters": "",
                "override": True,
            },
        },
    },
    ##############################################################################################
    "SERV": {
        "layer": "APP",
        "description": "Service",
        "interface": {"header": "Service/Service.hpp", "class": "Service"},
        "methods": {
            "init": {
                "return_type": "sys_error_t",
                "parameters": "void* params = nullptr",
                "override": True,
            },
            "start": {"return_type": "sys_error_t", "parameters": "", "override": True},
            "stop": {"return_type": "sys_error_t", "parameters": "", "override": True},
            "restart": {
                "return_type": "sys_error_t",
                "parameters": "",
                "override": True,
            },
        },
    },
}


# Helper function to generate method definitions
def get_method_definition(method_name, method_info, classname, for_header=True):
    """Generate method definition string for header or source file"""
    return_type = method_info["return_type"]
    parameters = method_info["parameters"]

    if for_header:
        override_keyword = " override" if method_info.get("override", False) else ""
        return f"{return_type} {method_name}({parameters}){override_keyword};"
    else:
        # For source file - remove default parameters
        params_clean = parameters.replace(" = nullptr", "").replace("= nullptr", "")
        return f"{return_type} {classname}::{method_name}({params_clean})"


def get_all_method_definitions(system_type, classname, for_header=True):
    """Get all method definitions for a system type"""
    if system_type not in SYSTEM_TYPES:
        return {}

    methods = SYSTEM_TYPES[system_type]["methods"]
    definitions = {}

    for method_name, method_info in methods.items():
        definitions[f"{method_name}_definition"] = get_method_definition(
            method_name, method_info, classname, for_header
        )

    return definitions
