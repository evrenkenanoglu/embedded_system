import sys
import subprocess
import ctypes
import re
import time  # <-- Added this for a slight delay

from Smart_Attach_Common import TARGET_VID_PID, is_admin, run_as_admin, find_bus_id, try_attach

def main():
    if not is_admin():
        run_as_admin()
        sys.exit(0)

    print(f"Searching for ESP32 (ID: {TARGET_VID_PID})...")

    bus_id = find_bus_id(TARGET_VID_PID)

    if not bus_id:
        print("\n[ERROR] ESP32 not found!")
        print("Please check if the device is plugged in.")
    else:
        print(f"Found ESP32 at Bus ID: {bus_id}")
        print("Attaching to WSL in the background...")

        try_attach(bus_id)

    print("\n" + "=" * 30)
    input("Press Enter to close this window...")

if __name__ == "__main__":
    main()