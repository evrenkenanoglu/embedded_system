import subprocess
import os

# Define the path to the html_esp32_copy.html script
pathScript = "C:\Workspace_Personal\ESP32\Embedded_IoT_BT_WIFI_Base_Project\embedded_system\Source\Scripts\html_to_c.py"

current_dir = os.path.dirname(os.path.abspath(__file__))
path_html_file = os.path.join(current_dir, "../output/html/ui_wifi_power_sockets_combined.html")
path_header_file = os.path.join(current_dir, "../output/header/ui_wifi_power_sockets.h")

# Call the script using subprocess
subprocess.call(["python", pathScript, path_html_file, path_header_file])
