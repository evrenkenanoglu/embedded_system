# config.py


# Matter Setup Parameters

# --- Device Identity ---
# Vendor ID (VID). 0xFFF1 is for "Test Vendor".
# For a pro product, you must acquire a VID from the CSA (Connectivity Standards Alliance).
VENDOR_ID = 0xFFF1

# Product ID (PID). 0x8000 is a "Test Product".
PRODUCT_ID = 0x8000

# --- Discovery Capabilities (Rendezvous) ---
# 2: BLE (Standard for ESP32/ESP32-C3/S3)
# 4: On-Network (Use this ONLY if you used your own WiFi Manager first)
RENDEZVOUS = 2

# --- Commissioning Flow ---
# 0: Standard (No user interaction required on device)
# 1: User Intent (User must press a button on device to allow pairing)
# 2: Custom
COMMISSIONING_FLOW = 0

# --- Versioning ---
VERSION = 0

CHIP_TOOL_PATH = "chip-tool"
