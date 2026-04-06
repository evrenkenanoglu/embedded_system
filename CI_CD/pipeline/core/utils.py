import subprocess
import sys
import zipfile
import shutil
import fnmatch
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
    print(
        f"{RED_BRIGHT}┃{RESET}{RED_BOLD}{title_text:^{width}}{RESET}{RED_BRIGHT}┃{RESET}"
    )

    # Divider
    print(f"{RED_BRIGHT}┠─{'─' * (width-2)}─┨{RESET}")

    # Time: The emoji ⏰ counts as 1 char in Python but 2 spaces in terminal.
    # We subtract 1 from the width to pull the right border back into line.
    time_display = f"⏰ {timestamp}"
    print(
        f"{RED_BRIGHT}┃{RESET}{SHINY_GREEN}{time_display:^{width-1}}{RESET}{RED_BRIGHT}┃{RESET}"
    )

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


def run_cmd(cmd, check=True, cwd=None):
    print(f"\n🚀 RUNNING: {' '.join(cmd)}")
    # The 'cwd' parameter tells the OS to start the process in that folder
    result = subprocess.run(cmd, cwd=cwd)
    if check and result.returncode != 0:
        sys.exit(result.returncode)


def create_package(source_dir, dest_dir, includes="*", excludes=None):

    print_stage(f"📦 CREATING PACKAGE FOLDER: {os.path.basename(dest_dir)} 📦")

    if os.path.exists(dest_dir):
        shutil.rmtree(dest_dir)
    os.makedirs(dest_dir)

    includes = [includes] if isinstance(includes, str) else (includes or ["*"])
    excludes = [excludes] if isinstance(excludes, str) else (excludes or [])
    count = 0

    # Helper function to handle the complex boolean logic
    def is_match(path, name, patterns):
        return any(
            fnmatch.fnmatch(path, p)  # Matches exact relative path OR wildcard
            or fnmatch.fnmatch(name, p)  # Matches filename wildcard (*.bin)
            or path.startswith(p.rstrip("/") + "/")  # Matches inside a specific folder
            for p in patterns
        )

    # Main traversal
    for root, _, files in os.walk(source_dir):
        for file in files:
            src_file = os.path.join(root, file)
            rel_path = os.path.relpath(src_file, source_dir)

            # --- 1 & 2. FILTERING ---
            if "*" not in includes and not is_match(rel_path, file, includes):
                continue
            if is_match(rel_path, file, excludes):
                continue

            # --- 3. COPY ---
            dest_file = os.path.join(dest_dir, rel_path)
            os.makedirs(os.path.dirname(dest_file), exist_ok=True)
            shutil.copy2(src_file, dest_file)
            count += 1
            print(f"  + Copied: {rel_path}")

    print(f"✅ Package created at: {dest_dir} ({count} files copied)")


def create_zip(source_dir, zip_name):
    print_stage(f" ⫘⫘⫘⫘⫘ ZIPPING PACKAGE: {os.path.basename(source_dir)}  ⫘⫘⫘⫘⫘ ")
    if os.path.exists(source_dir):
        shutil.make_archive(zip_name, "zip", source_dir)
    else:
        raise FileNotFoundError(f"Source dir {source_dir} not found!")


def extract_zip(zip_path, extract_to):
    print_stage(
        f" -ˋˏ✄┈┈┈ UNPACKING FIRMWARE ARTIFACT: {os.path.basename(zip_path)} -ˋˏ✄┈┈┈ "
    )
    if not os.path.exists(zip_path):
        raise FileNotFoundError(f"Zip file {zip_path} not found!")
    with zipfile.ZipFile(zip_path, "r") as zip_ref:
        zip_ref.extractall(extract_to)
