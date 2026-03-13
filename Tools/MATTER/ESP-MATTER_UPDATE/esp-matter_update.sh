#!/bin/bash
set -e

# --- Variables ---
IDF_PATH="$HOME/esp/v5.4.1/esp-idf"
MATTER_PATH="$HOME/esp/esp-matter"

# 1. Setup ESP-IDF
cd "$IDF_PATH"
source ./export.sh

# 2. Update or Clone esp-matter
if [ -d "$MATTER_PATH" ]; then
    cd "$MATTER_PATH"
    git pull
else
    git clone --depth 1 https://github.com/espressif/esp-matter.git "$MATTER_PATH"
    cd "$MATTER_PATH"
fi

# 3. Sync and Install
git submodule update --init --depth 1
cd ./connectedhomeip/connectedhomeip
./scripts/checkout_submodules.py --platform esp32 --shallow
cd ../..
./install.sh

echo "Done. Run 'source $MATTER_PATH/export.sh' to start."