import sys
import subprocess
import ctypes
import re
import time  # <-- Added this for a slight delay

# --- CONFIGURATION ---
TARGET_VID_PID = "10c4:ea60"
# ---------------------

def find_host_ip():
    """Finds the host's IP address for 'Ethernet adapter Ethernet 2'."""
    try:
        print("Finding host IP address for WSL...")
        # Capture output from ipconfig
        result = subprocess.check_output(["ipconfig"], text=True)
        
        # 1. Locate the specific 'Ethernet 2' block
        # This matches from 'Ethernet 2:' until it hits another 'adapter' header
        pattern = r"Ethernet adapter Ethernet 2:(.*?)(?=Ethernet adapter|Wireless LAN adapter|$)"
        host_section = re.search(pattern, result, re.DOTALL)
        
        if host_section:
            section_text = host_section.group(1)
            # 2. Extract the IPv4 Address within that block
            ip_match = re.search(r"IPv4 Address[.\s]*:\s*([\d.]+)", section_text)
            
            if ip_match:
                ip = ip_match.group(1)
                print(f"Host IP found: {ip}")
                return ip
            
        print("Host IP not found in 'Ethernet 2' section.")
    except Exception as e:
        print(f"[ERROR] Failed to find host IP: {e}")
    return None

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

def current_device_status(target_vid_pid):
    print("\n--- Current Device Status ---")
    try:
        subprocess.run(f"usbipd list | findstr {target_vid_pid}", shell=True)
    except:
        pass


def bind(bus_id):
    # We bind first (harmless if already bound) to ensure we have control
    subprocess.run(
        ["usbipd", "bind", "--busid", bus_id],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

def attach(bus_id, host_ip=None):
    cmd = ["usbipd", "attach", "--wsl", "--busid", bus_id, "--auto-attach"]
    if host_ip:
        cmd.extend(["--host-ip", host_ip])
    
    print(f"\nAttaching to WSL with command: {' '.join(cmd)}")
    CREATE_NO_WINDOW = 0x08000000
    subprocess.Popen(
        cmd,
        creationflags=CREATE_NO_WINDOW,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL
    )

def deattach(bus_id):
    return subprocess.run(["usbipd", "detach", "--busid", bus_id], text=True)



def try_attach(bus_id):
    """Attempts to bind and attach the device to WSL."""
    try:
            # 1. Bind (ignores error if already bound)
            bind(bus_id)


            # 2. Attach in the BACKGROUND using Popen
            attach(bus_id, host_ip=find_host_ip())

            print("\n[SUCCESS] Endless auto-attach loop started invisibly!")
            print("-" * 30)
            
            # Give usbipd 3 seconds to do the initial attach before checking the list
            print("Verifying connection...")
            time.sleep(3)
            subprocess.run(f"usbipd list | findstr {TARGET_VID_PID}", shell=True)

    except Exception as e:
            print(f"\n[CRITICAL ERROR] {e}")



def try_detach(bus_id):
    """Attempts to bind and detach the device from WSL."""
    try:
        # We bind first (harmless if already bound) to ensure we have control
        bind(bus_id)

        # Run the detach command
        result = deattach(bus_id)

        if result.returncode == 0:
            print("\n[SUCCESS] ESP32 is now detached from WSL.")
            print("Windows can now access the COM port.")
        else:
            print("\n[INFO] Device was likely already detached or not shared.")
            
    except Exception as e:
        print(f"\n[ERROR] An unexpected error occurred: {e}")

