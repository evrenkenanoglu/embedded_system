import subprocess
import time
import sys
import os
import argparse

SCRIPTS_DIR = os.path.join(os.path.dirname(__file__), "scripts")
SCRIPT_NAMES = [
    "generate_html_file.py",
    "generate_html_file_combined.py",
    "update_ui_welcome.py",
]


def run_scripts_once():
    for script in SCRIPT_NAMES:
        script_path = os.path.join(SCRIPTS_DIR, script)
        print(f"Running {script_path}...")
        subprocess.run([sys.executable, script_path])


def run_scripts_loop(period):
    try:
        while True:
            run_scripts_once()
            print(f"Waiting {period} seconds before next run...")
            time.sleep(period)
    except KeyboardInterrupt:
        print("Loop stopped by user.")


def main():
    parser = argparse.ArgumentParser(description="Run scripts in scripts/ directory.")
    parser.add_argument(
        "--loop",
        type=float,
        default=None,
        help="Run scripts in a loop with given period (seconds)",
    )
    args = parser.parse_args()

    if args.loop is not None:
        run_scripts_loop(args.loop)
    else:
        run_scripts_once()


if __name__ == "__main__":
    main()


# Show an example of how to run the script:
# python run.py --loop 10
