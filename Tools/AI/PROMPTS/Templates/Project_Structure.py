project_structure = """
Source/
|   ----HAL
|   |   ----Common
|   |   ----IHAL
|   |   \---Platform
|   |       \---ESP32
|   |           ----Library
|   |           \---Tests
|   |               \---Unit_Tests
|   |                   \---Fake
|   ----Library
|   |   ----Common
|   |   ----UI
|   |   |   \---HTTP
|   |   |       ----PowerSwitches
|   |   |       |   ----applications
|   |   |       |   |   ----css
|   |   |       |   |   ----js
|   |   |       |   |   ----output
|   |   |       |   |   |   ----header
|   |   |       |   |   |   \---html
|   |   |       |   |   ----scripts
|   |   |       |   |   \---template
|   |   |       |   \---ui_wifi_power_sockets
|   |   |       |       ----css
|   |   |       |       ----js
|   |   |       |       ----output
|   |   |       |       |   ----header
|   |   |       |       |   \---html
|   |   |       |       ----scripts
|   |   |       |       \---template
|   |   |       ----ui_welcome
|   |   |       |   ----css
|   |   |       |   ----output
|   |   |       |   |   ----header
|   |   |       |   |   ----html
|   |   |       |   |   \---js
|   |   |       |   ----scripts
|   |   |       |   ----template
|   |   |       |   \---test
|   |   |       \---ui_wifi_setup
|   |   |           ----css
|   |   |           ----js
|   |   |           ----output
|   |   |           |   ----header
|   |   |           |   \---html
|   |   |           ----scripts
|   |   |           \---template
|   |   \---Utility
|   ----PAL
|   |   ----Platform
|   |   |   \---Esp32
|   |   |       \---Protocol
|   |   |           ----HTTP
|   |   |           |   \---URIs
|   |   |           |       ----PowerSwitches
|   |   |           |       ----Welcome
|   |   |           |       \---WifiHome
|   |   |           ----IOT
|   |   |           \---MDNS
|   |   \---Protocols
|   |       ----Cloud_Service
|   |       ----HTTP
|   |       \---IOT
|   ----Process
|   |   \---Examples
|   |       ----Network
|   |       ----Peripheral
|   |       \---Protocol
|   ----Scripts
|   |   ----factory_generator
|   |   ----generator
|   |   |   \---cpp
|   |   |       \---__pycache__
|   |   ----html_to_c_converter
|   |   ----static_code_analysis
|   |   |   ----Analyzer
|   |   |   ----rules
|   |   |   ----Standards
|   |   |   \---testfiles
|   |   \---system_source_files_generator
|   |       \---templates
|   \---System
"""