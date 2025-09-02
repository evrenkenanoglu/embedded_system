from templates.System_Types import SYSTEM_TYPES
from templates.File_Generator import File_Generator

class FileSpec:
    def __init__(self):
        self.filename = ""
        self.classname = ""
        self.brief = ""
        self.system_type = ""

class UserInterface:
    def collect_file_specification(self):
        print("ESP32 System Source Files Generator")
        print("-" * 40)

        # Get filename
        filename = input("Enter filename: ").strip()

        # Show system types
        print(f"\nAvailable types: {', '.join(SYSTEM_TYPES.keys())}")

        # Get system type
        system_type = input("Enter system type: ").strip().upper()

        # Validate system typeF
        if system_type not in SYSTEM_TYPES:
            print(f"Invalid type. Using 'IO' as default.")
            system_type = "IO"

        # Generate classname BEFORE any case modifications
        classname = f"{system_type}_{filename}"

        # Get brief description
        brief = input("Enter brief description: ").strip()
        if not brief:
            brief = f"{SYSTEM_TYPES[system_type]['description']} implementation"

        # If it's HAL layer, lowercase the filename for file generation
        # but keep system_type uppercase for SYSTEM_TYPES lookup
        if SYSTEM_TYPES[system_type]["layer"] == "HAL":
            filename = filename.lower()
            # Update classname to use lowercase system_type for HAL
            classname = f"{system_type.lower()}_{filename}"

        # Create file spec
        file_spec = FileSpec()
        file_spec.filename = classname
        file_spec.classname = classname
        file_spec.brief = brief
        file_spec.system_type = system_type  # Keep uppercase for SYSTEM_TYPES lookup

        return file_spec



def main():
    ui = UserInterface()
    generator = File_Generator()

    # Collect user input
    file_spec = ui.collect_file_specification()

    print("File Specification:")
    print(f"  Filename: {file_spec.filename}")
    print(f"  Classname: {file_spec.classname}")
    print(f"  Brief: {file_spec.brief}")
    print(f"  System Type: {file_spec.system_type}")

    # Generate files
    generator.generate_files(file_spec)


if __name__ == "__main__":
    main()
