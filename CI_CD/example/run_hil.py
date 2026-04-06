#!/usr/bin/env python3
import sys
import os
import argparse

# Load configuration
import config
from config import ProjectConfig

# 3. Import shared mechanics
from pipeline.toolchains.factory import get_toolchain
from pipeline.core.utils import create_package
from pipeline.core.utils import extract_zip
from pipeline.core.utils import run_cmd
from pipeline.core.utils import print_stage
from pipeline.core.utils import print_start


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default=ProjectConfig.SERIAL_PORT)
    parser.add_argument("--method", choices=["usb", "ota"], default="usb")
    args = parser.parse_args()

    config = ProjectConfig()
    tc = get_toolchain(config)

    print_start("HIL TESTS")
    
    extract_zip(f"{config.ARTIFACT_NAME}.zip", config.EXTRACTED_PACKAGE_DIR)

    if args.method == "usb":
        tc.flash_usb(port= config.SERIAL_PORT, binary_dir=config.EXTRACTED_PACKAGE_DIR, flash_args= config.FLASH_ARGS_FILENAME)
    else:
        tc.flash_ota(args.port, config.EXTRACTED_PACKAGE_DIR, config.DEFAULT_OTA_PORT)

    # pytest -s --log-cli-level=INFO tests/hil/test_serial.py --port=COM3 --target=esp32 --embedded-services esp,serial

    print("\n--- STAGE: HIL TESTS ---")

    print("Current Directory:", os.getcwd())
    print("Project Root:", config.PROJECT_ROOT)
    print("HIL Test Directory:", config.HIL_TEST_DIR)

    test_file_name = "test_matter_hil.py"
    test_cmd = [
        "pytest",
        "-s",
        "-v",
        "--capture=tee-sys",
        "--log-cli-level=INFO",
        f"{config.HIL_TEST_DIR}/{test_file_name}",
        f"--port={args.port}",
        f"--target={config.TARGET}",
        f"--embedded-services={','.join(config.EMBEDDED_SERVICES)}",
        f"--html={config.TEST_OUTPUT_DIR}/{config.REPORT_FILE_PREFIX}_{test_file_name.replace('.py', '')}.html",
        "--self-contained-html",
    ]
    run_cmd(test_cmd, check=False)


if __name__ == "__main__":
    main()
