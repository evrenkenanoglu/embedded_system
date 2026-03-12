# config.py


# Matter Setup Parameters

# --- Commissioning Credentials ---
# The 12-bit discriminator (0-4095).
# This allows multiple devices to advertise simultaneously without conflict.
# IMPORTANT: Must match 'idf.py menuconfig' -> CHIP Device Layer -> Device Identification -> Default Discriminator
DISCRIMINATOR = 3840

# The 27-bit setup passcode (00000001-99999998).
# This acts as the password for the initial connection.
# IMPORTANT: Must match 'idf.py menuconfig' -> CHIP Device Layer -> Device Identification -> Default Setup Passcode
SETUP_PIN_CODE = 20202021

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