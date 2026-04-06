#!/usr/bin/env python3
import sys
import os

# 1. Load the project configuration
from config import ProjectConfig

if not os.path.exists(ProjectConfig.PIPELINE_LIB_PATH):
    sys.exit(1)
    
sys.path.append(ProjectConfig.PIPELINE_LIB_PATH)

from pipeline.toolchains.factory import get_toolchain
from pipeline.core.utils import create_zip, print_stage
from pipeline.core.git_manager import GitManager

def main():
    config = ProjectConfig()
    
    # --- STAGE 1: SOURCE CONTROL ---
    print_stage("SOURCE CONTROL SETUP")
    git = GitManager(config.WORKDIR)
    
    # If you need to clone from scratch, uncomment this:
    git.clone("https://github.com/evrenkenanoglu/Smart_Plugs_SW.git", branch="features/power_bar_app")
    
    # Update submodules (esp-matter, connectedhomeip, etc.)
    git.init_submodules()

    # --- STAGE 2: BUILD ---
    print_stage(f"BUILD (Target: {config.TARGET})")
    tc = get_toolchain(config)
    tc.build(config.TARGET)

    # --- STAGE 3: ARTIFACTS ---
    print_stage("PACKAGE ARTIFACTS")
    # create_zip(config.BUILD_DIR, config.ARTIFACT_NAME)
    
    print("\n✅ CI Pipeline Completed Successfully")

if __name__ == "__main__":
    main()