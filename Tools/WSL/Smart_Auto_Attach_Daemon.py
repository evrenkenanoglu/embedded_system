"""
Smart_Auto_Attach_Daemon.py
---------------------------
Continuously monitors USB ports for target ESP32 / USB-to-UART bridge devices.
Whenever a device is plugged in or returns to Windows (disconnected from WSL),
this daemon automatically re-binds and re-attaches it to WSL.

Features:
- Self-elevates to Administrator if required.
- Tracks connection state transitions to avoid spamming the console.
- Supports multiple simultaneous ESP32 boards.
- Includes debounce delays so rebooting microcontrollers can settle.
- Clean shutdown on Ctrl+C.
"""

import sys
import time
import subprocess
from Smart_Attach_Common import (
    TARGET_VID_PIDS,
    extract_vid_pid,
    is_admin,
    run_as_admin,
)

# Polling interval in seconds
POLL_INTERVAL = 1.5

# Settling delay (seconds) to let Windows finish driver enumeration before attaching
SETTLE_DELAY = 0.5


def get_all_target_devices(target_vid_pids: list[str]) -> list[dict]:
    """
    Parses 'usbipd list' and returns all matching target devices with their
    current Bus ID, VID:PID, and connection state.
    """
    devices = []
    target_set = {v.lower() for v in target_vid_pids}

    try:
        result = subprocess.check_output(["usbipd", "list"], text=True)
    except FileNotFoundError:
        print("[ERROR] 'usbipd' command not found. Please install usbipd-win.")
        input("Press Enter to exit...")
        sys.exit(1)
    except Exception as e:
        print(f"[ERROR] Failed to run 'usbipd list': {e}")
        return devices

    for line in result.splitlines():
        line_clean = line.strip()
        if not line_clean:
            continue

        vid_pid = extract_vid_pid(line_clean)
        if vid_pid and vid_pid in target_set:
            parts = line_clean.split()
            if not parts:
                continue

            bus_id = parts[0]
            line_lower = line_clean.lower()

            # Determine WSL connection state
            if "attached" in line_lower:
                state = "Attached"
            elif "not shared" in line_lower:
                state = "Not shared"
            elif "shared" in line_lower:
                state = "Shared"
            else:
                state = "Unknown"

            devices.append(
                {
                    "bus_id": bus_id,
                    "vid_pid": vid_pid,
                    "state": state,
                    "raw_line": line_clean,
                }
            )

    return devices


def attach_device_to_wsl(bus_id: str, vid_pid: str) -> bool:
    """
    Binds and attaches the specified device to WSL.
    Returns True if successfully attached, False otherwise.
    """
    try:
        # Step 1: Bind device (ensures it is shared with usbipd)
        subprocess.run(
            ["usbipd", "bind", "--busid", bus_id],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )

        # Step 2: Attach to WSL (direct attach without creating infinite sub-processes)
        cmd = ["usbipd", "attach", "--wsl", "--busid", bus_id]
        proc = subprocess.run(cmd, capture_output=True, text=True)

        if proc.returncode == 0:
            print(f"[SUCCESS] Attached Bus ID [{bus_id}] ({vid_pid}) to WSL.")
            return True
        else:
            err_msg = proc.stderr.strip() or proc.stdout.strip()
            print(f"[WARNING] Could not attach Bus ID [{bus_id}]: {err_msg}")
            return False

    except Exception as e:
        print(f"[ERROR] Failed attaching Bus ID [{bus_id}]: {e}")
        return False


def run_daemon() -> None:
    """Main monitoring loop."""
    print("=" * 60)
    print("     ESP32 -> WSL Persistent Auto-Attach Watchdog")
    print("=" * 60)
    print(f"Monitoring targets: {', '.join(TARGET_VID_PIDS)}")
    print("Status: WATCHING (Press Ctrl+C to stop)\n")

    # Cache last known state: {bus_id: state}
    last_states = {}

    while True:
        try:
            current_devices = get_all_target_devices(TARGET_VID_PIDS)
            current_bus_ids = {dev["bus_id"] for dev in current_devices}

            # 1. Report disconnected devices
            for bus_id in list(last_states.keys()):
                if bus_id not in current_bus_ids:
                    print(
                        f"[{time.strftime('%H:%M:%S')}] [DISCONNECTED] Device on Bus ID [{bus_id}] was removed."
                    )
                    del last_states[bus_id]

            # 2. Check current devices for attach readiness
            for dev in current_devices:
                bus_id = dev["bus_id"]
                vid_pid = dev["vid_pid"]
                state = dev["state"]
                prev_state = last_states.get(bus_id)

                # If state changed, log it once
                if state != prev_state:
                    print(
                        f"[{time.strftime('%H:%M:%S')}] [DETECTED] Bus ID [{bus_id}] ({vid_pid}) -> State: '{state}'"
                    )
                    last_states[bus_id] = state

                # If the device is connected to Windows but NOT attached to WSL
                if state in ("Not shared", "Shared"):
                    print(
                        f"[{time.strftime('%H:%M:%S')}] [ACTION] Device on Bus [{bus_id}] is on Windows. Attaching to WSL..."
                    )

                    # Short pause for microcontroller reset/USB handshake to complete
                    time.sleep(SETTLE_DELAY)

                    if attach_device_to_wsl(bus_id, vid_pid):
                        last_states[bus_id] = "Attached"

            time.sleep(POLL_INTERVAL)

        except KeyboardInterrupt:
            print("\n\n[EXIT] Watchdog stopped by user.")
            break
        except Exception as e:
            print(f"\n[UNEXPECTED LOOP ERROR]: {e}")
            time.sleep(POLL_INTERVAL)


def main() -> None:
    # 1. Self-elevate to Administrator
    if not is_admin():
        run_as_admin()
        sys.exit(0)

    # 2. Run monitoring daemon
    run_daemon()


if __name__ == "__main__":
    main()
