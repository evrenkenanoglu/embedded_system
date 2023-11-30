import sys
import os.path

# Read the HTML file
with open(sys.argv[1], 'r') as f:
    html_content = f.read()

# Remove line breaks from the content
html_content = html_content.replace('\n', '\\\n')
# html_content = html_content.replace('\n', ' ')

# Escape any double-quotes in the content
html_content = html_content.replace('"', '\\"')

# Get the output file name without the extension
output_file_name = os.path.splitext(sys.argv[2])[0]

# Wrap the content in a C-style header file string
c_style_html = f'#ifndef {output_file_name.upper()}_H\n#define {output_file_name.upper()}_H\n\n#define HTML_{output_file_name.upper()}_CONTENT "{html_content}"\n\n#endif'

# Write the result to the header file
with open(sys.argv[2], 'w') as f:
    f.write(c_style_html)

print(f'The output has been written to {sys.argv[2]}')

# Example command ->
# python html_to_c.py input.html output.h
# python ..\..\..\Scripts\html_to_c.py ui_welcome_wifi_connect.html ui_welcome_wifi_connect.h