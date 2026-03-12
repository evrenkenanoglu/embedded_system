#!/usr/bin/env python3
import argparse
import zipfile
import os
import threading
import http.server
import socketserver
import config
from core.utils import run_cmd
from toolchains.factory import get_toolchain


class HILPipeline:
    def __init__(self, port, flash_method, test_list):
        self.port = port
        self.flash_method = flash_method
        self.test_list = test_list
        self.toolchain = get_toolchain()

    def unpack(self):
        print("📂 STAGE: Unpack Firmware Artifact")
        artifact_file = f"{config.ARTIFACT_NAME}.zip"
        if not os.path.exists(artifact_file):
            raise FileNotFoundError(f"{artifact_file} missing! Run CI pipeline first.")

        with zipfile.ZipFile(artifact_file, "r") as zip_ref:
            zip_ref.extractall(config.EXTRACTED_BUILD_DIR)

    def flash(self):
        print(f"⚡ STAGE: Flashing via {self.flash_method.upper()}")
        
        if self.flash_method == "usb":
            self.toolchain.flash_usb(self.port, config.EXTRACTED_BUILD_DIR)
            
        elif self.flash_method == "ota":
            self.toolchain.flash_ota(self.port, config.EXTRACTED_BUILD_DIR, config.DEFAULT_OTA_PORT)

    def run_hil_tests(self):
        print("🌍 STAGE: Running HIL Tests")
        cmd = (
            ["pytest"]
            + self.test_list
            + [
                f"--port={self.port}",
                f"--html={config.REPORT_NAME}",
                "--self-contained-html",
            ]
        )
        run_cmd(cmd, check=False)