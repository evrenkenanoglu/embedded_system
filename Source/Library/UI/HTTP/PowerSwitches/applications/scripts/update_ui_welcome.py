import subprocess
import os

current_dir = os.path.dirname(os.path.abspath(__file__))

# Define the path to the html_esp32_copy.html script
pathScript = os.path.join(current_dir, "../../../../../../Scripts/html_to_c_converter/html_to_c.py")

# Get the folder of the one level up from the current script
filename = os.path.basename(os.path.dirname(current_dir))

path_html_file = os.path.join(current_dir, f"../output/html/{filename}_combined.html")
path_header_file = os.path.join(current_dir, f"../output/header/{filename}.h")

# Call the script using subprocess
subprocess.call(["python", pathScript, path_html_file, path_header_file])
