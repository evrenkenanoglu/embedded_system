import sys
import subprocess
import ctypes
import re

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
    # 'runas' is the Windows verb for "Run as Administrator"
    ctypes.windll.shell32.ShellExecuteW(
        None, "runas", sys.executable, " ".join(sys.argv), None, 1
    )


def find_bus_id(target_vid_pid):
    """Runs 'usbipd list' and parses output to find the Bus ID."""
    try:
        # Run usbipd list and capture output
        result = subprocess.check_output(["usbipd", "list"], text=True)

        # Parse line by line
        for line in result.splitlines():
            if target_vid_pid in line:
                # The Bus ID is usually the first element (e.g., "2-2")
                parts = line.split()
                if parts:
                    return parts[0]
    except FileNotFoundError:
        print("[ERROR] 'usbipd' command not found. Please install usbipd-win.")
        input("Press Enter to exit...")
        sys.exit(1)
    except Exception as e:
        print(f"[ERROR] Failed to scan devices: {e}")
    return None


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
        print("Attaching to WSL...")

        try:
            # 1. Bind (ignores error if already bound)
            subprocess.run(
                ["usbipd", "bind", "--busid", bus_id],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )

            # 2. Attach
            cmd = ["usbipd", "attach", "--wsl", "--busid", bus_id, "--auto-attach"]
            result = subprocess.run(cmd, text=True)

            if result.returncode == 0:
                print("\n[SUCCESS] Device attached successfully!")
                print("-" * 30)
                # Show verification
                subprocess.run(f"usbipd list | findstr {TARGET_VID_PID}", shell=True)
            else:
                print("\n[ERROR] Failed to attach. The device might be busy.")

        except Exception as e:
            print(f"\n[CRITICAL ERROR] {e}")

    print("\n" + "=" * 30)
    input("Press Enter to close this window...")


if __name__ == "__main__":
    main()
