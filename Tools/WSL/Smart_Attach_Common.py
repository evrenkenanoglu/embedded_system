"""
Smart_Attach_Common.py
----------------------
Core library and common utilities for managing ESP32 / USB-UART serial
devices attached to WSL2 using usbipd-win.

Can also be executed standalone to automatically find and attach the first
matching device.
"""

import ctypes
import os
import re
import subprocess
import sys
import time

# --- CONFIGURATION ---
# Known USB-to-UART Bridge Chip VID:PIDs:
# - 303a:1001 = ESP32-C6 / ESP32-S3 Native USB-Serial/JTAG
# - 1a86:55d3 = CH343 / CH340 series
# - 10c4:ea60 = Silicon Labs CP2102 / CP2104
# - 0403:6001 = FTDI FT232R
TARGET_VID_PIDS = [
    "303a:1001",
    "1a86:55d3",
    "10c4:ea60",
    "0403:6001",
]

# Backward compatibility alias
TARGET_VID_PID = TARGET_VID_PIDS


def extract_vid_pid(text: str) -> str | None:
    """
    Extracts vendor and product ID (VID:PID) from a line of text.

    Supports:
      - Standard hex format: '10c4:ea60'
      - Windows DeviceID format: 'VID_10C4&PID_EA60'

    Returns:
        Lowercase string formatted as 'vid:pid', or None if not matched.
    """
    # Check standard hex format (e.g., 303a:1001)
    match_std = re.search(r"([0-9a-fA-F]{4}):([0-9a-fA-F]{4})", text)
    if match_std:
        return f"{match_std.group(1)}:{match_std.group(2)}".lower()

    # Check Windows DeviceID format (e.g., VID_303A&PID_1001)
    match_win = re.search(
        r"VID_([0-9a-fA-F]{4})&PID_([0-9a-fA-F]{4})", text, re.IGNORECASE
    )
    if match_win:
        return f"{match_win.group(1)}:{match_win.group(2)}".lower()

    return None


def is_admin() -> bool:
    """Checks whether the current script process has Windows Administrator privileges."""
    try:
        return ctypes.windll.shell32.IsUserAnAdmin() != 0
    except Exception:
        return False


def run_as_admin() -> None:
    """Restarts the current script with elevated Administrator privileges."""
    print("Requesting Administrator privileges...")
    try:
        # Wrap script path and arguments in quotes to handle directory names with spaces
        params = " ".join([f'"{arg}"' for arg in sys.argv])
        ctypes.windll.shell32.ShellExecuteW(
            None, "runas", sys.executable, params, None, 1
        )
    except Exception as e:
        print(f"[ERROR] Failed to elevate privileges: {e}")
        input("Press Enter to exit...")


def find_target_device(vid_pid_list: list[str]) -> tuple[str | None, str | None]:
    """
    Scans devices using 'usbipd list' and matches against the provided VID:PID list.

    Returns:
        tuple (bus_id, matched_vid_pid) if found, otherwise (None, None).
    """
    try:
        result = subprocess.check_output(["usbipd", "list"], text=True)
        target_set = {v.lower() for v in vid_pid_list}

        for line in result.splitlines():
            line_vid_pid = extract_vid_pid(line)
            if line_vid_pid and line_vid_pid in target_set:
                parts = line.split()
                if parts:
                    bus_id = parts[0]
                    return bus_id, line_vid_pid

    except FileNotFoundError:
        print("[ERROR] 'usbipd' command not found. Please install usbipd-win.")
        input("Press Enter to exit...")
        sys.exit(1)
    except Exception as e:
        print(f"[ERROR] Failed to scan devices: {e}")

    return None, None


def find_bus_id(target_vid_pid: str | list[str]) -> str | None:
    """
    Convenience wrapper to return only the BUS ID.
    Accepts either a single VID:PID string or a list of VID:PID strings.
    """
    if isinstance(target_vid_pid, str):
        target_list = [target_vid_pid]
    else:
        target_list = target_vid_pid

    bus_id, _ = find_target_device(target_list)
    return bus_id


def current_device_status(vid_pid_list: str | list[str] | None = None) -> None:
    """
    Prints the current status of relevant USB devices from 'usbipd list'.
    Accepts None (uses default TARGET_VID_PIDS), a single string, or a list.
    """
    print("\n--- Current Connected Devices ---")

    if vid_pid_list is None:
        targets = TARGET_VID_PIDS
    elif isinstance(vid_pid_list, str):
        targets = [vid_pid_list]
    else:
        targets = vid_pid_list

    target_set = {v.lower() for v in targets}

    try:
        result = subprocess.check_output(["usbipd", "list"], text=True)
        found_any = False
        for line in result.splitlines():
            vid_pid = extract_vid_pid(line)
            if vid_pid and vid_pid in target_set:
                print(line)
                found_any = True

        if not found_any:
            print("No matching devices found in 'usbipd list'.")
    except Exception as e:
        print(f"[ERROR] Unable to fetch device status: {e}")


def bind(bus_id: str) -> subprocess.CompletedProcess:
    """Binds the specified bus ID with usbipd so it can be shared with WSL."""
    return subprocess.run(
        ["usbipd", "bind", "--busid", bus_id],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )


def attach(bus_id: str) -> None:
    """
    Attaches the specified bus ID to WSL with persistent auto-attach enabled.
    Spawns invisibly without opening a secondary console window.
    """
    cmd = ["usbipd", "attach", "--wsl", "--busid", bus_id, "--auto-attach"]
    print(f"\nAttaching BUS [{bus_id}] to WSL with command: {' '.join(cmd)}")
    create_no_window = 0x08000000
    subprocess.Popen(
        cmd,
        creationflags=create_no_window,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )


def deattach(bus_id: str) -> subprocess.CompletedProcess:
    """Detaches the specified bus ID from WSL."""
    return subprocess.run(["usbipd", "detach", "--busid", bus_id], text=True)


def try_attach(bus_id: str, matched_vid_pid: str | None = None) -> None:
    """Attempts to bind and auto-attach the device to WSL."""
    try:
        vid_pid_display = matched_vid_pid if matched_vid_pid else "Target Device"
        print(f"Targeting device [{vid_pid_display}] on Bus ID [{bus_id}]")

        bind(bus_id)
        attach(bus_id)

        print("\n[SUCCESS] Auto-attach background process started.")
        print("-" * 35)

        print("Verifying connection in 3 seconds...")
        time.sleep(3)
        current_device_status()

    except Exception as e:
        print(f"\n[CRITICAL ERROR] Failed during attach: {e}")


def try_detach(bus_id: str) -> None:
    """Attempts to detach the device from WSL and return it to Windows."""
    try:
        bind(bus_id)
        result = deattach(bus_id)

        if result.returncode == 0:
            print(f"\n[SUCCESS] Bus ID {bus_id} is now detached from WSL.")
            print("Windows can now access the COM port directly.")
        else:
            print(f"\n[INFO] Device (Bus ID {bus_id}) was already detached or not shared.")

    except Exception as e:
        print(f"\n[ERROR] An error occurred while detaching: {e}")


if __name__ == "__main__":
    if not is_admin():
        run_as_admin()
        sys.exit(0)

    bus_id, matched_vid_pid = find_target_device(TARGET_VID_PIDS)

    if bus_id:
        try_attach(bus_id, matched_vid_pid)
    else:
        print("\n[INFO] No target ESP32/Serial device found connected.")
        current_device_status()

    input("\nPress Enter to exit...")