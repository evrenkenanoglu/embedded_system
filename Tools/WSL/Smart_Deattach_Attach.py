import sys
import subprocess
import ctypes
import time

# --- CONFIGURATION ---
TARGET_VID_PID = "10c4:ea60"
# ---------------------


def is_admin():
    """Checks if the script is running with Administrator privileges."""
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except:
        return False


def run_as_admin():
    """Restarts the current script with Administrator privileges."""
    print("Requesting Administrator privileges...")
    try:
        # 'runas' is the Windows verb for "Run as Administrator"
        ctypes.windll.shell32.ShellExecuteW(
            None, "runas", sys.executable, " ".join(sys.argv), None, 1
        )
    except Exception as e:
        print(f"Error elevating privileges: {e}")
        input("Press Enter to exit...")


def find_bus_id(target_vid_pid):
    """Runs 'usbipd list' and finds the Bus ID for the given VID:PID."""
    try:
        # Run usbipd list
        result = subprocess.check_output(["usbipd", "list"], text=True)

        # Parse output line by line
        for line in result.splitlines():
            if target_vid_pid in line:
                # The Bus ID is the first part of the line (e.g., "2-2")
                parts = line.split()
                if parts:
                    return parts[0]
    except FileNotFoundError:
        print("[ERROR] 'usbipd' command not found. Is usbipd-win installed?")
    except Exception as e:
        print(f"[ERROR] Failed to scan devices: {e}")
    return None


def bind(bus_id):
    # We bind first (harmless if already bound) to ensure we have control
    subprocess.run(
        ["usbipd", "bind", "--busid", bus_id],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )


def deattach(bus_id):
    # Run the detach command
    result = subprocess.run(["usbipd", "detach", "--busid", bus_id], text=True)
    return result


def attach(bus_id):
    # Run the attach command
    cmd = ["usbipd", "attach", "--wsl", "--busid", bus_id, "--auto-attach"]
    result = subprocess.run(cmd, text=True)
    return result


def current_device_status(target_vid_pid):
    print("\n--- Current Device Status ---")
    try:
        subprocess.run(f"usbipd list | findstr {target_vid_pid}", shell=True)
    except:
        pass


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
    try:
        bind(bus_id)
        result = deattach(bus_id)
        if result.returncode == 0:
            print("\n[SUCCESS] ESP32 is now detached from WSL.")
        else:
            print("\n[ERROR] Failed to detach. The device might be busy.")

        if result.returncode == 0:
            print("\n[SUCCESS] ESP32 is now detached from WSL.")
            print("Windows can now access the COM port.")

        else:
            print("\n[INFO] Device was likely already detached or not shared.")

    except Exception as e:
        print(f"\n[ERROR] An unexpected error occurred: {e}")

    # Sleep 1 sec
    time.sleep(1)
    # Attach again
    try:
        bind(bus_id)
        result = attach(bus_id)
        if result.returncode == 0:
            print("\n[SUCCESS] ESP32 is now attached to WSL.")
        else:
            print("\n[ERROR] Failed to attach. The device might be busy.")

    except Exception as e:
        print(f"\n[ERROR] An unexpected error occurred: {e}")

    # 4. List current status
    current_device_status(TARGET_VID_PID)

    print("\n")
    # 5. Wait for user input before closing
    input("Press Enter to close this window...")


if __name__ == "__main__":
    main()
