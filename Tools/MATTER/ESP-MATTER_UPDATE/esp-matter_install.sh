MATTER_VERSION="v1.4.2"
MATTER_BRANCH="release/$MATTER_VERSION"
ESP_PATH="$HOME/esp"
IDF_PATH="$ESP_PATH/v5.4.1/esp-idf"
MATTER_PATH="$ESP_PATH/esp-matter-$MATTER_VERSION"

cd $ESP_PATH

# Remove existing esp-matter directory if it exists
if [ -d "$MATTER_PATH" ]; then
    rm -rf "$MATTER_PATH"
    echo "Removed existing esp-matter directory."
else
    echo "esp-matter directory does not exist."
fi

# Setup ESP-IDF
source "$IDF_PATH/export.sh"

# Clone esp-matter
git clone https://github.com/espressif/esp-matter.git "$MATTER_PATH"
cd "$MATTER_PATH"

# Checkout the specific release branch
git branch -a | grep $MATTER_VERSION
git tag -l | grep $MATTER_VERSION

git checkout origin/$MATTER_BRANCH -b $MATTER_BRANCH

git submodule update --init --depth 1

cd ./connectedhomeip/connectedhomeip
./scripts/checkout_submodules.py --platform esp32 linux --shallow
cd ../..
./install.sh
cd ..

