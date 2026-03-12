import subprocess
import sys
import zipfile
import shutil
import os

from datetime import datetime

def print_test_info(test_name, status):
    print(f"🧪 {test_name}: {status}")
    print(f"⏰ Time: {datetime.now().strftime('%H:%M:%S')}")
    print()

def print_start(test_name):
    RED_BOLD = "\033[1;31m"
    RED_BRIGHT = "\033[91m"
    DIM = "\033[2m"
    RESET = "\033[0m"
    SHINY_GREEN = "\033[1;32m"

    width = 70
    timestamp = datetime.now().strftime("%H:%M:%S")
    
    # Header Frame
    print(f"\n\n{RED_BRIGHT}┏━{'━' * (width-2)}━┓{RESET}")

    # Title: ❯❯ (2 chars) + space + name + space + ❮❮ (2 chars) = 6 extra chars
    title_text = f"❯❯ {test_name.upper()} ❮❮"
    print(f"{RED_BRIGHT}┃{RESET}{RED_BOLD}{title_text:^{width}}{RESET}{RED_BRIGHT}┃{RESET}")

    # Divider
    print(f"{RED_BRIGHT}┠─{'─' * (width-2)}─┨{RESET}")

    # Time: The emoji ⏰ counts as 1 char in Python but 2 spaces in terminal.
    # We subtract 1 from the width to pull the right border back into line.
    time_display = f"⏰ {timestamp}"
    print(f"{RED_BRIGHT}┃{RESET}{SHINY_GREEN}{time_display:^{width-1}}{RESET}{RED_BRIGHT}┃{RESET}")

    # Bottom Glow
    print(f"{RED_BRIGHT}┗━{'━' * (width-2)}━┛{RESET}\n")


def print_stage(stage_name):
    """
    Prints a highly visible, colored banner for pipeline stages.
    """
    # ANSI Escape Codes for colors
    CYAN = "\033[1;36m"
    WHITE_BOLD = "\033[1;37m"
    RESET = "\033[0m"

    width = 70
    line = "━" * width

    # 2 empty lines before, 1 empty line after for maximum breathability in logs
    print(f"\n\n{CYAN}{line}{RESET}")
    print(f"{CYAN} ❯ {WHITE_BOLD}STAGE: {stage_name.upper()}{RESET}")
    print(f"{CYAN}{line}{RESET}\n")


def run_cmd(cmd, check=True):
    print(f"\n🚀 RUNNING: {' '.join(cmd)}")
    result = subprocess.run(cmd)
    if check and result.returncode != 0:
        sys.exit(result.returncode)


def create_zip(source_dir, zip_name):
    print_stage("📦 PACKAGE ARTIFACT 📦")
    if os.path.exists(source_dir):
        shutil.make_archive(zip_name, "zip", source_dir)
    else:
        raise FileNotFoundError(f"Source dir {source_dir} not found!")


def extract_zip(zip_path, extract_to):
    print_stage(" -ˋˏ✄┈┈┈ UNPACKING FIRMWARE ARTIFACT -ˋˏ✄┈┈┈ ")
    with zipfile.ZipFile(zip_path, "r") as zip_ref:
        zip_ref.extractall(extract_to)
