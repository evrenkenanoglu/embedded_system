import sys
import subprocess
import ctypes
import time

from Smart_Attach_Common import is_admin, run_as_admin, find_bus_id, try_detach, TARGET_VID_PID

def main():
    # 1. Self-elevate
    if not is_admin():
        run_as_admin()
        sys.exit(0)

    print(f"Searching for ESP32 (ID: {TARGET_VID_PID})...")
    
    # 2. Find BUSID
    bus_id = find_bus_id(TARGET_VID_PID)

    if not bus_id:
        print("\n[ERROR] ESP32 not found!")
        print("Please make sure the device is plugged in.")
        input("\nPress Enter to exit...")
        sys.exit(1)

    print(f"Found ESP32 at Bus ID: {bus_id}")
    print("Detaching from WSL (returning to Windows)...")

    # 3. Detach Logic
    try_detach(bus_id)
    
    # 4. List current status
    print("\n--- Current Device Status ---")
    try:
        subprocess.run(f"usbipd list | findstr {TARGET_VID_PID}", shell=True)
    except:
        pass

    print("\n")
    # 5. Wait for user input before closing
    input("Press Enter to close this window...")

if __name__ == "__main__":
    main()