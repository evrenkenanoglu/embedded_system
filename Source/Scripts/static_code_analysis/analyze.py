import argparse
from pathlib import Path
from Analyzer.analyzer import Analyzer

# Import standard classes
from Standards.Analysis_Standard_Base import Analysis_Standard_Base
from Standards.Simple_Test_Standard import SimpleTestStandard

# from Standards.CERT_Standard import CERTStandard

# Add more imports as you create more standards
# from Standards.MISRA_Standard import MISRAStandard
# from Standards.IoT_Standard import IoTStandard
# from Standards.ESP32_Standard import ESP32Standard


def create_available_standards():
    """Create instances of all available standards"""
    rules_dir = Path(__file__).parent / "rules"

    standards = []

    # Add Simple Test Standard
    standards.append(SimpleTestStandard(rules_dir))

    # Add more standards as you implement them
    # standards.append(CERTStandard(script_dir))
    # standards.append(MISRAStandard(script_dir))
    # standards.append(IoTStandard(script_dir))
    # standards.append(ESP32Standard(script_dir))

    return standards


def activate_standards(available_standards, requested_standards):
    """Activate standards based on user input"""
    for standard in available_standards:
        for requested_std in requested_standards:
            if requested_std.lower() in standard.name.lower():
                standard.is_active = True
                print(f"🔵 Activated: {standard.name}")
                break


def main():
    parser = argparse.ArgumentParser(description="ESP32 IoT Static Code Analysis")
    parser.add_argument("--file", "-f", help="File to analyze")
    parser.add_argument(
        "--standards",
        "-s",
        nargs="+",
        choices=["simple", "cert", "misra", "iot", "esp32"],
        default=["simple"],
        help="Standards to activate (default: simple)",
    )
    parser.add_argument(
        "--list", "-l", action="store_true", help="List available standards"
    )

    args = parser.parse_args()

    # Check file args empty, if yes import all_files from sourcefiles.py
    if not args.file:
        print("⚠️ All files will be analyzed.")
        from sourcefiles import all_files

        args.file = all_files
    else:
        # Ensure the file exists
        target_file = Path(args.file)
        if not target_file.exists():
            print(f"❌ File '{args.file}' does not exist!")
            return

        # If a single file is provided, convert it to a list
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
        print("   python analyze.py --standards simple --file test.cpp")
        print("   python analyze.py --standards cert misra --file code.cpp")
        return


    # Create analyzer with all standards
    analyzer = Analyzer(target_file=args.file, standard_list=available_standards)

    # Activate requested standards
    activate_standards(available_standards, args.standards)

    print("🚀 ESP32 IoT Static Code Analysis")
    print("=" * 50)
    analyzer.print_status()
    print()

    success = analyzer.run_analysis()

    if success:
        print("\n✅ Analysis Complete!")
    else:
        print("\n❌ Analysis Failed!")


if __name__ == "__main__":
    main()
