#!/usr/bin/env python3
import sys
import os
import argparse

# 1. Load configuration
from config import ProjectConfig

# 2. Inject library path
if not os.path.exists(ProjectConfig.PIPELINE_LIB_PATH):
    print(
        f"❌ Error: Central Pipeline library not found at {ProjectConfig.PIPELINE_LIB_PATH}"
    )
    sys.exit(1)

sys.path.append(ProjectConfig.PIPELINE_LIB_PATH)

# 3. Import shared mechanics
from pipeline.toolchains.factory import get_toolchain
from pipeline.core.utils import extract_zip, run_cmd, print_stage, print_start


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default=ProjectConfig.DEFAULT_PORT)
    parser.add_argument("--method", choices=["usb", "ota"], default="usb")
    args = parser.parse_args()

    config = ProjectConfig()
    tc = get_toolchain(config)

    print_start("HIL TESTS")
    
    # extract_zip(f"{config.ARTIFACT_NAME}.zip", config.EXTRACTED_BUILD_DIR)

    # if args.method == "usb":
    #     tc.flash_usb(args.port, config.EXTRACTED_BUILD_DIR)
    # else:
    #     tc.flash_ota(args.port, config.EXTRACTED_BUILD_DIR, config.DEFAULT_OTA_PORT)

    print("\n--- STAGE: HIL TESTS ---")
    test_cmd =[
        "pytest", 
        "-s", 
        config.HIL_TEST_DIR, 
        f"--port={args.port}", 
        f"--target={config.TARGET}",  # <--- ADD THIS SO IT RESETS THE CHIP!
        "--html=hil_report.html", 
        "--self-contained-html"
    ]
    run_cmd(test_cmd, check=False)


if __name__ == "__main__":
    main()
