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


def return_inline_scripts_from_directory(directory):
    inline_scripts = []
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(".js"):
                js_file_path = os.path.join(root, file)
                try:
                    with open(js_file_path, "r", encoding="utf-8") as js_file:
                        js_content = js_file.read()
                        # Remove comment lines starting with //
                        js_content = "\n".join(
                            line
                            for line in js_content.splitlines()
                            if not line.strip().startswith("//")
                        )
                        inline_scripts.append(js_content)
                except Exception as e:
                    print(f"Error reading JS file {js_file_path}: {e}")

    # Join all inline scripts into a single script block
    inline_scripts = [script.strip() for script in inline_scripts if script.strip()]
    if not inline_scripts:
        inline_scripts = ["console.log('No inline scripts found');"]

    inline_scripts = "\n    ".join(
        [f"<script>\n{script}\n</script>" for script in inline_scripts]
    )

    return inline_scripts


def return_inline_styles_from_directory(directory):
    inline_styles = []
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(".css"):
                css_file_path = os.path.join(root, file)
                try:
                    with open(css_file_path, "r", encoding="utf-8") as css_file:
                        css_content = css_file.read()
                        inline_styles.append(css_content)
                except Exception as e:
                    print(f"Error reading CSS file {css_file_path}: {e}")

    # Join all inline styles into a single style block
    inline_styles = [style.strip() for style in inline_styles if style.strip()]
    if not inline_styles:
        inline_styles = ["body { background-color: #f0f0f0; }"]
    inline_styles = "\n    ".join(
        [f"<style>\n{style}\n</style>" for style in inline_styles]
    )
    return inline_styles


if __name__ == "__main__":
    current_dir = os.path.dirname(os.path.abspath(__file__))
    filename = os.path.basename(os.path.dirname(current_dir))
    # change the current directory to parent directory one level up
    current_dir = os.path.join(current_dir, "..")
    
    template_path = os.path.join(current_dir, "template/template.html")
    output_path = os.path.join(current_dir, f'output/html/{filename}_combined.html')
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

    inline_scripts = return_inline_scripts_from_directory(js_directory)
    inline_styles = return_inline_styles_from_directory(current_dir)


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

        formatter = TabHTMLFormatter(indent=4)  # 4 tab per level
        formatted_html = soup.prettify(formatter=formatter)

        with open(output_path, "w", encoding="utf-8") as file:
            file.write(formatted_html)

        print(f"Formatted HTML file saved to: {output_path}")
    except ImportError:
        print("BeautifulSoup is not installed. Skipping formatting step.")
    except Exception as e:
        print(f"Error formatting HTML: {e}")
