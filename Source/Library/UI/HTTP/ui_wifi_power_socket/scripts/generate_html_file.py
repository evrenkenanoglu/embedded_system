import os


def read_external_resources_return_list(file_path):
    try:
        with open(file_path, "r", encoding="utf-8") as file:
            return [line.strip() for line in file if line.strip()]
    except Exception as e:
        print(f"Error reading external resources file {file_path}: {e}")
        return []


def fill_template(template_path, output_path, placeholders):
    try:
        with open(template_path, "r", encoding="utf-8") as file:
            template_content = file.read()

        # Replace placeholders with actual content
        for key, value in placeholders.items():
            placeholder = "{" + key + "}"
            template_content = template_content.replace(placeholder, value)

        # Write the filled template to the output file
        with open(output_path, "w", encoding="utf-8") as file:
            file.write(template_content)

        print(f"Filled template saved to: {output_path}")
    except Exception as e:
        print(f"Error processing template: {e}")


def return_inline_script_files_from_directory(directory):
    # return only .js file names list, not their content
    inline_scripts = []
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(".js"):
                # Get the relative path from the input directory
                rel_path = os.path.relpath(os.path.join(root, file), directory)
                # Replace backslashes with forward slashes for proper web paths
                rel_path = rel_path.replace('\\', '/')
                # Create script tag with the relative path
                script_tag = f'<script src="js/{rel_path}"></script>'
                inline_scripts.append(script_tag)
    
    # Sort the script tags to ensure consistent ordering
    inline_scripts.sort()
    
    # Join all script tags into a single block
    inline_scripts_str = "\n    ".join(inline_scripts)
    
    if not inline_scripts:
        print("No JavaScript files found in the specified directory.")
    
    return inline_scripts_str


def return_inline_style_files_from_directory(directory):
    # return only .css file names list, not their content
    inline_styles = []
    css_directory = os.path.join(directory, "css")
    
    if os.path.exists(css_directory):
        for root, _, files in os.walk(css_directory):
            for file in files:
                if file.endswith(".css"):
                    # Get the relative path from the css directory
                    rel_path = os.path.relpath(os.path.join(root, file), directory)
                    # Replace backslashes with forward slashes for proper web paths
                    rel_path = rel_path.replace('\\', '/')
                    # Create link tag with the relative path
                    style_tag = f'<link rel="stylesheet" href="{rel_path}">'
                    inline_styles.append(style_tag)
    
    # Sort the style tags to ensure consistent ordering
    inline_styles.sort()
    
    # Join all style tags into a single block
    inline_styles_str = "\n    ".join(inline_styles)
    
    if not inline_styles:
        print("No CSS files found in the specified directory.")
    
    return inline_styles_str


if __name__ == "__main__":
    current_dir = os.path.dirname(os.path.abspath(__file__))
    # change current_dir to the parent directory one level up
    current_dir = os.path.join(current_dir, "..")

    template_path = os.path.join(current_dir, "template/template.html")
    output_path = os.path.join(current_dir, "ui_wifi_power_sockets.html")
    stylesheet_list_path = os.path.join(current_dir, "css", "styleSheetlist.txt")
    script_list_path = os.path.join(current_dir, "js", "scriptList.txt")
    js_directory = os.path.join(current_dir, "js")

    # Read external stylesheets from file
    stylesheet_list = read_external_resources_return_list(stylesheet_list_path)
    external_stylesheets = "\n    ".join(
        [f'<link rel="stylesheet" href="{resource}">' for resource in stylesheet_list]
    )

    # Read external scripts from file
    script_list = read_external_resources_return_list(script_list_path)
    external_scripts = "\n    ".join(
        [f'<script src="{resource}"></script>' for resource in script_list]
    )

    inline_scripts = return_inline_script_files_from_directory(js_directory)
    inline_styles = return_inline_style_files_from_directory(current_dir)


    # Define placeholder values
    placeholders = {
        "external_stylesheets": external_stylesheets,
        "inline_styles": inline_styles,
        "external_scripts": external_scripts,
        "inline_scripts": inline_scripts,
    }

    fill_template(template_path, output_path, placeholders)

    # Format the output HTML file
    try:
        from bs4 import BeautifulSoup
        from bs4.formatter import HTMLFormatter

        class TabHTMLFormatter(HTMLFormatter):
            def indent(self, level):
                return "\t" * level

        with open(output_path, "r", encoding="utf-8") as file:
            soup = BeautifulSoup(file, "lxml")

        formatter = TabHTMLFormatter(indent=1)  # 1 tab per level
        formatted_html = soup.prettify(formatter=formatter)

        with open(output_path, "w", encoding="utf-8") as file:
            file.write(formatted_html)

        print(f"Formatted HTML file saved to: {output_path}")
    except ImportError:
        print("BeautifulSoup is not installed. Skipping formatting step.")
    except Exception as e:
        print(f"Error formatting HTML: {e}")
