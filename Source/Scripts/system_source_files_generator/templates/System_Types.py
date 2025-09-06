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
    ##############################################################################################
    "IO_GPIO": {
        "layer": "HAL",
        "description": "GPIO Input/Output",
        "interface": {"header": "IHal_Io_Gpio.h", "class": "IHAL_IO_GPIO"},
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
            "setDirection": {
                "return_type": "sys_error_t",
                "parameters": "hal_gpio_direction_t direction",
                "override": True,
            },
            "getDirection": {
                "return_type": "sys_error_t",
                "parameters": "hal_gpio_direction_t& direction",
                "override": True,
            },
            "setPull": {
                "return_type": "sys_error_t",
                "parameters": "hal_gpio_pull_t pull",
                "override": True,
            },
            "getPull": {
                "return_type": "sys_error_t",
                "parameters": "hal_gpio_pull_t& pull",
                "override": True,
            },
            "setInterrupt": {
                "return_type": "sys_error_t",
                "parameters": "hal_gpio_interrupt_t interrupt",
                "override": True,
            },
            "getInterrupt": {
                "return_type": "sys_error_t",
                "parameters": "hal_gpio_interrupt_t& interrupt",
                "override": True,
            },
            "setInterruptHandler": {
                "return_type": "sys_error_t",
                "parameters": "void (*handler)(void* params), void* params",
                "override": True,
            },
            "enableInterrupt": {
                "return_type": "sys_error_t",
                "parameters": "",
                "override": True,
            },
            "disableInterrupt": {
                "return_type": "sys_error_t",
                "parameters": "",
                "override": True,
            },
            "clearInterrupt": {
                "return_type": "sys_error_t",
                "parameters": "",
                "override": True,
            },
            "getLevel": {
                "return_type": "sys_error_t",
                "parameters": "hal_gpio_level_t& level",
                "override": True,
            },
            "setLevel": {
                "return_type": "sys_error_t",
                "parameters": "hal_gpio_level_t level",
                "override": True,
            },
            "toggleLevel": {
                "return_type": "sys_error_t",
                "parameters": "",
                "override": True,
            },
            "getEventQueue": {
                "return_type": "void*",
                "parameters": "",
                "override": True,
            },
            "getGpioNumber": {
                "return_type": "uint16_t",
                "parameters": "",
                "override": True,
                "const": True,
            },
            "getPortNumber": {
                "return_type": "uint8_t",
                "parameters": "",
                "override": True,
                "const": True,
            },
            "hasCapability": {
                "return_type": "bool",
                "parameters": "uint32_t capability",
                "override": True,
                "const": True,
            },
        },
    },
}


# Helper function to generate method definitions
def get_method_definition(method_name, method_info, classname, for_header=True):
    """Generate method definition string for header or source file"""
    return_type = method_info["return_type"]
    parameters = method_info["parameters"]

    # Get function attributes
    const_qualifier = " const" if method_info.get("const", False) else ""
    virtual_qualifier = "virtual " if method_info.get("virtual", True) else ""
    static_qualifier = "static " if method_info.get("static", False) else ""
    inline_qualifier = "inline " if method_info.get("inline", False) else ""

    if for_header:
        override_keyword = " override" if method_info.get("override", False) else ""
        pure_virtual = " = 0" if method_info.get("pure_virtual", False) else ""

        # Build the complete method signature
        qualifiers = f"{static_qualifier}{inline_qualifier}{virtual_qualifier}"
        return f"{qualifiers}{return_type} {method_name}({parameters}){const_qualifier}{override_keyword}{pure_virtual};"
    else:
        # For source file - remove default parameters and virtual/override keywords
        params_clean = parameters.replace(" = nullptr", "").replace("= nullptr", "")
        qualifiers = f"{static_qualifier}{inline_qualifier}"
        return f"{qualifiers}{return_type} {classname}::{method_name}({params_clean}){const_qualifier}"


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
