import sys

import pytest
import pytest_embedded
import pytest_embedded_idf
import pytest_embedded_serial
import pytest_embedded_serial_esp

import subprocess
import re
import time
import serial
import os


current_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.abspath(os.path.join(current_dir, "../../"))

print (f"Current Directory: {current_dir}")
print (f"Project Root: {project_root}")

if project_root not in sys.path:
    sys.path.insert(0, project_root)

from config import ProjectConfig as Config
# USAGE:
# pytest test_matter_hil.py -v --capture=tee-sys --html=report.html --self-contained-html

# --- CONFIGURATION ---
NODE_ID = "1"
SETUP_PIN_CODE = "20202021"
ENDPOINTS = ["1", "2", "3", "4"]

# Parameterized Log Patterns
CONSOLE_INIT_PATTERN = r"\[DEBUG\] Initializing Matter Console!"

CONSOLE_MATTER_CMD_LIST = {
    "factory_reset": f"matter device factoryreset",
    "wifi_connect": f"matter esp wifi connect {Config.TARGET_WIFI_SSID} {Config.TARGET_WIFI_PASSWORD}",
}

CHIP_TOOL_CMD_LIST = {
    "pair_on_network": f"{Config.CHIP_TOOL_PATH} pairing onnetwork {NODE_ID} {SETUP_PIN_CODE}",
    "unpair": f"{Config.CHIP_TOOL_PATH} pairing unpair {NODE_ID}",
    "read_basic_info": f"{Config.CHIP_TOOL_PATH} basicinformation read vendor-name {NODE_ID} {ENDPOINTS[0]}",
}

def run_chip_tool(*args, ignore_errors=False):
    """Executes chip-tool commands and returns the output."""
    cmd =[Config.CHIP_TOOL_PATH] + list(args)
    print(f"\n[CHIP-TOOL] {' '.join(cmd)}")
    
    result = subprocess.run(cmd, capture_output=True, text=True)
    output = result.stdout + result.stderr
    
    if result.returncode != 0 and not ignore_errors:
        pytest.fail(f"chip-tool failed (Code {result.returncode}).\nOutput:\n{output}")
        
    return output


# Global variable to store the dynamically found IP
DEVICE_IP = None

class TestMatterHIL:

    def test_00_connect_wifi_via_serial(self, dut: pytest_embedded.Dut):
        """
        Connects to the ESP32 via USB Serial.
        - Waits for console (parameterized log message)
        - Factory resets
        - Waits for reboot (parameterized log message)
        - Pushes Wi-Fi credentials
        - Captures the new IP Address
        """
        global DEVICE_IP
        print(f"\n[SERIAL] Opening {Config.SERIAL_PORT}...")
        
        try:

            # 1. Wait until matter console is initialized
            print(f"\n[STEP 1] Waiting for Matter console to be ready...")
            # We look for the parameterized INIT pattern OR the CLI prompt ">" in case it booted 10 mins ago
            ready_pattern = f"({CONSOLE_INIT_PATTERN}|>)"
            dut.expect(ready_pattern, timeout=10)

            # 2. Send factory reset command
            cmd = f"{CONSOLE_MATTER_CMD_LIST['factory_reset']}\r\n"
            print(f"\n[STEP 2] Sending: {CONSOLE_MATTER_CMD_LIST['factory_reset']}")
            dut.write(cmd.encode('utf-8'))

            # 3. Wait again until matter console is initialized after the reboot
            print("\n[STEP 3] Waiting for ESP32 to reboot and initialize Matter...")
            # Strictly wait for the boot log this time (no newline spam needed)
            dut.expect(CONSOLE_INIT_PATTERN, timeout=25)
            
            # 4. Send the Wi-Fi connect command
            cmd = f"{CONSOLE_MATTER_CMD_LIST['wifi_connect']}\r\n"
            print(f"\n[STEP 4] Sending: {CONSOLE_MATTER_CMD_LIST['wifi_connect']}")
            dut.write(cmd.encode('utf-8'))

            # 5. Wait until WIFI_EVENT_STA_CONNECTED / got ip (increased timeout to 30s for retries)
            print("\n[STEP 5] Waiting for Wi-Fi connection and IP Address...")
            match = dut.expect(re.compile(r"(?:got ip:|ip:\s*)(\d+\.\d+\.\d+\.\d+)"), timeout=30)

            # 6. Save device IP locally (Handle both byte-strings and regular strings safely)
            raw_ip = match.group(1)
            DEVICE_IP = raw_ip.decode('utf-8') if isinstance(raw_ip, bytes) else raw_ip
            
            print(f"\n[SUCCESS] Device fully provisioned on Wi-Fi with IP: {DEVICE_IP}\n")
        except Exception as e:
            pytest.fail(f"Serial communication failed. Is 'idf.py monitor' still running? Error: {e}")


    def test_01_commission_device(self):
        assert DEVICE_IP, "Device IP not found!"
        
        for attempt in range(3):    
            output = run_chip_tool("pairing", "onnetwork", NODE_ID, SETUP_PIN_CODE)
            
            # Print the raw chip-tool output to the console so you can read it!
            print(f"\n--- CHIP-TOOL OUTPUT (Attempt {attempt+1}) ---\n{output}\n---------------------------")
            
            success = re.search(r"Device commissioning completed with success", output, re.IGNORECASE)
            if success:
                print("\n[SUCCESS] Device commissioned successfully!")
                break
            else:
                print(f"[WARNING] Commissioning attempt {attempt+1} failed.")
                time.sleep(5)
        else:
            pytest.fail("All commissioning attempts failed.")
    
    def test_02_verify_connectivity(self):
        """Read the Basic Information cluster to ensure communication."""
        output = run_chip_tool("basicinformation", "read", "vendor-name", NODE_ID, "0")
        assert "Espressif" in output or "VendorName" in output, f"Failed to read Basic Information cluster! Output: {output}"

    def test_03_find_endpoints(self):
        """Reads the list of endpoints to ensure we can discover them.
            Example output lines to look for:
                [1774113732.704] [38885:38918] [TOO] Endpoint: 0 Cluster: 0x0000_001D Attribute 0x0000_0003 DataVersion: 1212745918
                [1774113732.704] [38885:38918] [TOO]   PartsList: 4 entries
                [1774113732.704] [38885:38918] [TOO]     [1]: 1
                [1774113732.704] [38885:38918] [TOO]     [2]: 2
                [1774113732.704] [38885:38918] [TOO]     [3]: 3
                [1774113732.704] [38885:38918] [TOO]     [4]: 4
        """
        # ./chip-tool descriptor read parts-list <node_id> 0
        output = run_chip_tool("descriptor", "read", "parts-list", NODE_ID, "0")
        
        print(f"\n[ENDPOINT DISCOVERY OUTPUT]\n{output}\n---------------------------")
        # Look for lines that indicate the PartsList attribute with entries, and capture the endpoint numbers
        parts_list_match = re.search(r"PartsList:\s*(\d+)\sentries", output)
        assert parts_list_match, "Failed to find PartsList in descriptor read output!"
        num_entries = int(parts_list_match.group(1))
        assert num_entries > 0, "PartsList has no entries!"
        endpoint_matches = re.findall(r"\[\d+\]:\s*(\d+)", output)
        assert len(endpoint_matches) == num_entries, f"Expected {num_entries} endpoints but found {len(endpoint_matches)}!"
        print(f"\n[SUCCESS] Found {num_entries} endpoints: {endpoint_matches}\n")
    
    def test_04_read_on_all_endpoints(self):
        """Reads the Basic Information cluster on all discovered endpoints to ensure they are responsive.
            Example output lines to look for:
                [1774114071.017] [41226:41258] [TOO]   OnOff: FALSE       
        """
        for ep in ENDPOINTS:
            # chip-tool onoff read on-off <node_id> <endpoint_id>
            output = run_chip_tool("onoff", "read", "on-off", NODE_ID, ep)
            print(f"\n[READ ON/OFF OUTPUT - Endpoint {ep}]\n{output}\n---------------------------")
            onoff_match = re.search(r"OnOff:\s*(\w+)", output)
            assert onoff_match, f"Failed to read OnOff cluster on endpoint {ep}! Output: {output}"
            print(f"\n[SUCCESS] Endpoint {ep} is responsive with OnOff state: {onoff_match.group(1)}\n")
    
    def test_05_set_on_all_endpoints(self):
        """ 
            Sets the OnOff cluster to ON (1) on all endpoints,
            Then 1 second delay

            Example output lines to look for for ON:
                [1774114655.399] [44294:44325] [DMG]                            CommandPathIB =
                [1774114655.399] [44294:44325] [DMG]                            {
                [1774114655.399] [44294:44325] [DMG]                                    EndpointId = 0x1,
                [1774114655.399] [44294:44325] [DMG]                                    ClusterId = 0x6,
                [1774114655.399] [44294:44325] [DMG]                                    CommandId = 0x1,
                [1774114655.399] [44294:44325] [DMG]                            },
                [1774114655.399] [44294:44325] [DMG] 
                [1774114655.399] [44294:44325] [DMG]                            StatusIB =
                [1774114655.399] [44294:44325] [DMG]                            {
                [1774114655.399] [44294:44325] [DMG]                                    status = 0x00 (SUCCESS),
                [1774114655.399] [44294:44325] [DMG]                            },
        """
        for ep in ENDPOINTS:
            # chip-tool onoff on <node_id> <endpoint_id>
            output = run_chip_tool("onoff", "on", NODE_ID, ep)
            print(f"\n[WRITE ON/OFF OUTPUT - Endpoint {ep}]\n{output}\n---------------------------")
            success = re.search(r"status\s*=\s*0x00\s*\(SUCCESS\)", output)
            assert success, f"Failed to write OnOff cluster on endpoint {ep}! Output: {output}"
            print(f"\n[SUCCESS] Endpoint {ep} OnOff set to 1 successfully!\n")


        time.sleep(1)

    
    def test_06_set_off_all_endpoints(self):
        """
            Sets the OnOff cluster to OFF (0) on all endpoints,
            Then 1 second delay

            Example output lines to look for for OFF:
        """
        for ep in ENDPOINTS:
            output = run_chip_tool("onoff", "off", NODE_ID, ep)
            print(f"\n[WRITE ON/OFF OUTPUT - Endpoint {ep}]\n{output}\n---------------------------")
            success = re.search(r"status\s*=\s*0x00\s*\(SUCCESS\)", output)
            assert success, f"Failed to write OnOff cluster on endpoint {ep}! Output: {output}"
            print(f"\n[SUCCESS] Endpoint {ep} OnOff set to 0 successfully!\n")

        time.sleep(1)



    

    
        

    