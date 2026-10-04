"""
Smart_Attach.py
---------------
Finds the target ESP32/Serial device and attaches it to WSL2 with auto-attach.
Elevates to Administrator privileges automatically if required.
"""

import sys
from Smart_Attach_Common import (
    TARGET_VID_PIDS,
    is_admin,
    run_as_admin,
    find_target_device,
    try_attach,
)


def main() -> None:
    # 1. Elevate to Administrator if necessary
    if not is_admin():
        run_as_admin()
        sys.exit(0)

    print("Searching for connected ESP32 / Serial devices...")

    # 2. Find target device (Bus ID and VID:PID)
    bus_id, matched_vid_pid = find_target_device(TARGET_VID_PIDS)

    if not bus_id:
        print("\n[ERROR] ESP32 device not found!")
        print("Please check that your board is securely plugged into USB.")
    else:
        print(f"Found target device [{matched_vid_pid}] at Bus ID: {bus_id}")
        print("Attaching to WSL in the background...")
        try_attach(bus_id, matched_vid_pid)

    print("\n" + "=" * 35)
    input("Press Enter to close this window...")


if __name__ == "__main__":
    main()
