import os
from datetime import datetime
from templates.System_Types import SYSTEM_TYPES, get_all_method_definitions
from templates.cpp_templates import get_header_template, get_source_template


class File_Generator:
    def __init__(self, output_dir="./output"):
        self.output_dir = output_dir
        self.author = "Evren Kenanoglu"
        self.copyright_year = "2023-"

    def generate_files(self, file_spec):
        """Generate both header and source files"""
        try:
            # Ensure output directory exists
            os.makedirs(self.output_dir, exist_ok=True)

            # Generate file contents
            header_content = self._generate_header(file_spec)
            source_content = self._generate_source(file_spec)

            # Write files
            header_path = os.path.join(self.output_dir, f"{file_spec.filename}.hpp")
            source_path = os.path.join(self.output_dir, f"{file_spec.filename}.cpp")

            with open(header_path, "w") as f:
                f.write(header_content)

            with open(source_path, "w") as f:
                f.write(source_content)

            print(f"✅ Generated: {header_path}")
            print(f"✅ Generated: {source_path}")

        except Exception as e:
            print(f"❌ Error generating files: {e}")

    def _generate_header(self, file_spec):
        """Generate header file content"""
        system_config = SYSTEM_TYPES[file_spec.system_type]
        interface_header = system_config["interface"]["header"]
        interface_class = system_config["interface"]["class"]

        # Get method definitions for header
        method_defs = get_all_method_definitions(
            file_spec.system_type, file_spec.classname, for_header=True
        )
        methods_section = "\n    ".join([def_text for def_text in method_defs.values()])

        current_date = datetime.now().strftime("%d/%m/%Y")
        header_guard = f"{file_spec.filename.upper()}_HPP"

        # Use template with formatting
        template = get_header_template()
        return template.format(
            filename=file_spec.filename.lower(),
            brief=file_spec.brief,
            copyright_year=self.copyright_year,
            author=self.author,
            current_date=current_date,
            header_guard=header_guard,
            interface_header=interface_header,
            classname=file_spec.classname,
            interface_class=interface_class,
            methods_section=methods_section,
        )

    def _generate_source(self, file_spec):
        """Generate source file content"""
        # Get method definitions for source
        method_defs = get_all_method_definitions(
            file_spec.system_type, file_spec.classname, for_header=False
        )

        # Generate method implementations
        methods_impl = []

        for method_name, method_def in method_defs.items():
            method_name_clean = method_name.replace("_definition", "")

            method_impl = f"""
    {method_def.replace(';', '')}
    {{
        // TODO: Implement {method_name_clean}
    }}"""
            methods_impl.append(method_impl)

        methods_section = "\n".join(methods_impl)
        current_date = datetime.now().strftime("%d/%m/%Y")

        # Use template with formatting
        template = get_source_template()
        return template.format(
            filename=file_spec.filename.lower(),
            brief=file_spec.brief,
            copyright_year=self.copyright_year,
            author=self.author,
            current_date=current_date,
            classname=file_spec.classname,
            methods_section=methods_section,
        )
