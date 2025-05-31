import subprocess

# Define the path to the html_esp32_copy.html script
pathScript = 'C:\Workspace_Personal\ESP32\Embedded_IoT_BT_WIFI_Base_Project\embedded_system\Source\Scripts\html_to_c.py'
pathFile1 = 'ui_wifi_power_sockets.html'
pathFile2 = 'ui_wifi_power_sockets.h'

# Call the script using subprocess
subprocess.call(['python', pathScript, pathFile1, pathFile2])