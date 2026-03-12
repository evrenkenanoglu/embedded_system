#!/usr/bin/env python3
import argparse
import shutil
import os
import config
from core.utils import run_cmd
from toolchains.factory import get_toolchain


class CIPipeline:
    def __init__(self, target):
        self.target = target
        self.toolchain = get_toolchain()  # Automatically loads ESP-IDF, Zephyr, etc.

    def clone_and_init(self):
        print("📦 STAGE: Init Submodules")
        run_cmd(["git", "submodule", "update", "--init", "--recursive"])

    def build(self):
        print(f"🔨 STAGE: Build Application ({self.target})")
        self.toolchain.build(self.target)

    def run_unit_tests(self):
        print("🧪 STAGE: Host-based Unit Tests")
        self.toolchain.run_unit_tests()

    def checks(self):
        print("🔍 STAGE: Static Analysis")
        self.toolchain.run_static_checks()

    def package(self):
        print("📦 STAGE: Package Firmware Artifact")
        build_dir = os.path.join(config.PROJECT_ROOT, "build")
        if os.path.exists(build_dir):
            shutil.make_archive(config.ARTIFACT_NAME, "zip", build_dir)
            print(f"✅ Created {config.ARTIFACT_NAME}.zip")
        else:
            print("❌ Build folder not found!")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--target", default=config.DEFAULT_TARGET)
    args = parser.parse_args()

    ci = CIPipeline(args.target)
    ci.clone_and_init()
    ci.build()
    ci.run_unit_tests()
    ci.checks()
    ci.package()
