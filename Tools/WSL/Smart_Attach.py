import sys
import subprocess
import ctypes
import re
import time  # <-- Added this for a slight delay

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
    ctypes.windll.shell32.ShellExecuteW(
        None, "runas", sys.executable, " ".join(sys.argv), None, 1
    )

def find_bus_id(target_vid_pid):
    """Runs 'usbipd list' and parses output to find the Bus ID."""
    try:
        result = subprocess.check_output(["usbipd", "list"], text=True)
        for line in result.splitlines():
            if target_vid_pid in line:
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
        print("Attaching to WSL in the background...")

        try:
            # 1. Bind (ignores error if already bound)
            subprocess.run(["usbipd", "bind", "--busid", bus_id],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )

            # 2. Attach in the BACKGROUND using Popen
            cmd =["usbipd", "attach", "--wsl", "--busid", bus_id, "--auto-attach"]
            
            # Windows flag to run the process without a console window
            CREATE_NO_WINDOW = 0x08000000
            
            subprocess.Popen(
                cmd,
                creationflags=CREATE_NO_WINDOW,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL
            )

            print("\n[SUCCESS] Endless auto-attach loop started invisibly!")
            print("-" * 30)
            
            # Give usbipd 3 seconds to do the initial attach before checking the list
            print("Verifying connection...")
            time.sleep(3)
            subprocess.run(f"usbipd list | findstr {TARGET_VID_PID}", shell=True)

        except Exception as e:
            print(f"\n[CRITICAL ERROR] {e}")

    print("\n" + "=" * 30)
    input("Press Enter to close this window...")

if __name__ == "__main__":
    main()