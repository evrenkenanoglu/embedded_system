import argparse
from pathlib import Path
from Analyzer.analyzer import Analyzer

# Import standard classes
from Standards.Analysis_Standard_Base import Analysis_Standard_Base
from Standards.Simple_Test_Standard import SimpleTestStandard
from Standards.MISRA_Standard import Misra_Standard
from Standards.CERT_Standard import Cert_Standard
from Standards.IoT_Standard import IoT_Standard
from Standards.ESP32_Standard import ESP32_Standard


def create_available_standards():
    """Create instances of all available standards"""
    rules_dir = Path(__file__).parent / "rules"

    standards = []

    # Add Simple Test Standard
    standards.append(Cert_Standard(rules_dir))
    standards.append(Misra_Standard(rules_dir))
    standards.append(IoT_Standard(rules_dir))
    standards.append(ESP32_Standard(rules_dir))

    return standards


def activate_standards(available_standards, requested_standards):
    """Activate standards based on user input"""
    activated = []
    for standard in available_standards:
        for requested_std in requested_standards:
            if requested_std.lower() in standard.name.lower():
                standard.is_active = True
                activated.append(standard.name)
                print(f"🔵 Activated: {standard.name}")
                break

    if not activated:
        print(f"⚠️  No standards activated from: {requested_standards}")


def main():
    parser = argparse.ArgumentParser(description="ESP32 IoT Static Code Analysis")
    parser.add_argument("--file", "-f", help="File to analyze")
    parser.add_argument(
        "--standards",
        "-s",
        nargs="+",
        choices=["simple", "cert", "misra", "iot", "esp32"],
        default=["iot"],
        help="Standards to activate (default: simple)",
    )
    parser.add_argument(
        "--list", "-l", action="store_true", help="List available standards"
    )

    args = parser.parse_args()

    # Check file args empty, if yes import all_files from sourcefiles.py
    if not args.file:
        print("⚠️ All files will be analyzed.")
        try:
            from sourcefiles import all_files

            # Convert all files to Path objects
            args.file = [Path(f) for f in all_files]
        except ImportError:
            print("❌ sourcefiles.py not found. Please specify a file with --file")
            return
    else:
        # Ensure the file exists and convert to Path
        target_file = Path(args.file)
        if not target_file.exists():
            print(f"❌ File '{args.file}' does not exist!")
            return
        # Convert single file to list of Path objects
        args.file = [target_file]

    # Create all available standards
    available_standards = create_available_standards()

    if not available_standards:
        print("❌ No standards could be created! Check your Standards directory.")
        return

    if args.list:
        print("📋 Available Standards:")
        for standard in available_standards:
            standard.print_status()
        print("\nUsage:")
        print("   python main.py --standards simple --file test.cpp")
        print("   python main.py --standards cert misra --file code.cpp")
        print("   python main.py")
        return

    # Create analyzer
    analyzer = Analyzer()

    # Activate requested standards
    activate_standards(available_standards, args.standards)

    print("🚀 ESP32 IoT Static Code Analysis")
    print("=" * 50)

    # Process each file
    for file_path in args.file:
        print(f"\n🔍 Analyzing file: {file_path}")
        analyzer.print_status(file_path, available_standards)
        print()

        success = analyzer.run_analysis(
            file_path, available_standards, output_format="sarif"
        )

        if success:
            print(f"✅ Analysis Complete for {file_path.name}")
        else:
            print(f"❌ Analysis Failed for {file_path.name}")

        print("-" * 50)  # Separator between files


if __name__ == "__main__":
    main()
