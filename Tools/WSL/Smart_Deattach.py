"""
Smart_Deattach.py
-----------------
Detaches the ESP32/Serial device from WSL2 so Windows can immediately access
the COM port (e.g. for Windows-native flashing or serial monitors).
"""

import sys
from Smart_Attach_Common import (
    TARGET_VID_PIDS,
    is_admin,
    run_as_admin,
    find_target_device,
    try_detach,
    current_device_status,
)


def main() -> None:
    # 1. Self-elevate to Administrator
    if not is_admin():
        run_as_admin()
        sys.exit(0)

    print("Searching for connected ESP32 / Serial devices...")

    # 2. Locate target device
    bus_id, matched_vid_pid = find_target_device(TARGET_VID_PIDS)

    if not bus_id:
        print("\n[ERROR] ESP32 device not found!")
        print("Please ensure the board is plugged in.")
        input("\nPress Enter to exit...")
        sys.exit(1)

    print(f"Found device [{matched_vid_pid}] at Bus ID: {bus_id}")
    print("Detaching from WSL (releasing port back to Windows)...")

    # 3. Perform detach
    try_detach(bus_id)

    # 4. Show updated status
    current_device_status()

    print("\n" + "=" * 35)
    input("Press Enter to close this window...")


if __name__ == "__main__":
    main()