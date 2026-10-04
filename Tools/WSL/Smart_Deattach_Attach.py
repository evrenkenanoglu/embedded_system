"""
Smart_Deattach_Attach.py
------------------------
Power-cycles / reconnects the ESP32 connection to WSL.
Detaches the device, waits 1 second, and reattaches it.
Useful when serial ports lock up or open-port handles hang inside WSL.
"""

import sys
import time
from Smart_Attach_Common import (
    TARGET_VID_PIDS,
    is_admin,
    run_as_admin,
    find_target_device,
    try_detach,
    try_attach,
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

    # 3. Detach step (release back to Windows)
    print("\n[STEP 1/2] Detaching from WSL...")
    try_detach(bus_id)

    # 4. Short settle delay
    print("Waiting 1 second before reattaching...")
    time.sleep(1)

    # 5. Re-attach step
    print("\n[STEP 2/2] Re-attaching to WSL...")
    try_attach(bus_id, matched_vid_pid)

    # 6. Display final status
    current_device_status()

    print("\n" + "=" * 35)
    input("Press Enter to close this window...")


if __name__ == "__main__":
    main()
