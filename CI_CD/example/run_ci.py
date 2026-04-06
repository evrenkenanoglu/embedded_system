#!/usr/bin/env python3
import sys
import os

# 1. Load the project configuration
from config import ProjectConfig

if not os.path.exists(ProjectConfig.PIPELINE_LIB_PATH):
    sys.exit(1)

sys.path.append(ProjectConfig.PIPELINE_LIB_PATH)

from pipeline.core.docker_manager import DockerManager
from pipeline.toolchains.factory import get_toolchain
from pipeline.core.utils import create_package
from pipeline.core.utils import create_zip
from pipeline.core.utils import print_stage
from pipeline.core.utils import run_cmd

from pipeline.core.git_manager import GitManager


def main():
    # =================================================
    # SETUP
    # =================================================
    config = ProjectConfig()
    docker_manager = DockerManager(config.PROJECT_ROOT)
    tc = get_toolchain(config, docker_manager)

    # =================================================
    # CLEANUP
    # =================================================
    docker_manager.cleanup_path(config.TEMP_WORKSPACE_NAME)
    # remove the artifacts
    if os.path.exists(config.EXTRACTED_PACKAGE_DIR):
        print(f"🧹 Removing existing extracted package at {config.EXTRACTED_PACKAGE_DIR}")
        run_cmd(["rm", "-rf", config.EXTRACTED_PACKAGE_DIR])
    
    if os.path.exists(config.ARTIFACT_NAME + ".zip"):
        print(f"🧹 Removing existing artifact at {config.ARTIFACT_NAME}.zip")
        run_cmd(["rm", "-rf", config.ARTIFACT_NAME + ".zip"])

    # =================================================
    # STAGE: SOURCE CONTROL
    # =================================================
    print_stage("SOURCE CONTROL SETUP")
    git = GitManager(config.WORKDIR)
    git.clone(
        "https://github.com/evrenkenanoglu/Smart_Plugs_SW.git",
        branch="features/power_bar_app",
    )
    git.init_submodules()

    # =================================================
    # STAGE: BUILD
    # =================================================
    tc.build(target=config.TARGET, image_bin=config.FACTORY_BIN_PATH)

    # =================================================
    # STAGE: EXTRACT ARTIFACTS
    # =================================================

    create_package(
        source_dir=config.BUILD_DIR_PATH,
        dest_dir=config.PACKAGE_DIR,
        includes=config.PACKAGE_INCLUDES,
    )

    create_zip(
        source_dir=config.PACKAGE_DIR,
        zip_name=config.ARTIFACT_NAME,
    )

    print("\n✅ CI Pipeline Completed Successfully")


if __name__ == "__main__":
    main()
