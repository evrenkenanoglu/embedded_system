#!/usr/bin/env python3
import sys
import os


# 1. Load the project configuration
from config import ProjectConfig

# 2. Inject the central CI_CD library path into Python's module search path
if not os.path.exists(ProjectConfig.PIPELINE_LIB_PATH):
    print(f"❌ Error: Central Pipeline library not found at {ProjectConfig.PIPELINE_LIB_PATH}")
    print("Please check your folder structure or set the PIPELINE_LIB_PATH environment variable.")
    sys.exit(1)
    
sys.path.append(ProjectConfig.PIPELINE_LIB_PATH)

# 3. NOW we can safely import the shared mechanics!
from pipeline.toolchains.factory import get_toolchain
from pipeline.core.utils import create_zip, print_stage

def main():
    config = ProjectConfig()
    tc = get_toolchain(config) # Pass config into the library

    # tc.build(config.TARGET)
    # tc.run_unit_tests()
    create_zip(config.BUILD_DIR, config.ARTIFACT_NAME)
    print("✅ CI Pipeline Completed Successfully")

if __name__ == "__main__":
    main()