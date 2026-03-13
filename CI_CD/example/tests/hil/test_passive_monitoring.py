import pytest
from pytest_embedded import Dut
from pytest_embedded_serial_esp import EspSerial

def test_firmware_and_system_boot(dut: Dut):
    """
    Test 1: Verify the correct firmware is loaded and core systems initialize.
    """
    
    # 1. Verify ESP-IDF bootloader and partition
    # Using a slightly looser regex so it survives ESP-IDF updates (removed the "v5.4.1-dirty" part)
    dut.expect(r'boot: ESP-IDF.*2nd stage bootloader', timeout=10)
    dut.expect(r'boot: Loaded app from partition', timeout=5)
    
    # 2. Verify Application details
    dut.expect(r'Project name:\s+Embedded_IoT_BT_WIFI_Base_Proje', timeout=5)
    
    # 3. Verify core devices and ISR initialization
    dut.expect(r'Devices initialized successfully', timeout=5)
    dut.expect(r'GPIO ISR service initialized successfully', timeout=5)
    
    # 4. Verify the GPIO Expander setup completely finishes
    dut.expect(r'Initialized GPIO Expander Pin: 7 on Port: 1', timeout=5)


def test_wifi_connection_sequence(dut: Dut):
    """
    Test 2: Verify the device successfully reads credentials, connects to WiFi, and gets an IP.
    """
    # 1. Check for credential retrieval
    dut.expect(r'WiFi Credentials retrieved from memory:', timeout=10)
    
    # 2. Verify Station mode starts
    dut.expect(r'Starting STA mode...', timeout=5)
    dut.expect(r'WiFi Started!', timeout=5)
    
    # 3. Wait for actual network connection and IP assignment
    dut.expect(r'Connected to the network', timeout=15)
    
    # We use a regex to match ANY valid IP address assigned by the router
    dut.expect(r'IP: \d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}', timeout=5)


def test_https_server_startup(dut: Dut):
    """
    Test 3: Verify the internal web server starts and routes are registered correctly.
    """
    dut.expect(r'Starting server', timeout=15)
    dut.expect(r'HTTPS server started successfully!', timeout=5)
    
    # Verify critical endpoints are registered
    dut.expect(r'Registered URI /welcome', timeout=2)
    dut.expect(r'Registered URI /power-switches-control', timeout=2)


def test_physical_button_and_switch_events(dut: Dut):
    """
    Test 4: Monitor physical hardware interactions.
    This test will block until a button is physically pressed or triggered externally.
    """
    # 1. Wait for someone (or an external test fixture) to press Button Index 5
    # Setting a high timeout (e.g., 60 seconds) to give time for the physical event to occur
    print("\n[WAITING] Please trigger Button 5 to continue the test...")
    dut.expect(r'\[DEBUG\] Received button event: Index=5, Event=0', timeout=60)
    
    # 2. Verify the system executes the command to turn Switch 4 ON (State=1)
    dut.expect(r'\[DEBUG\] Executing switch command: Switch Index=4, Target State=1', timeout=5)
    dut.expect(r'\[DEBUG\] Setting switch 4 to state 1', timeout=5)
    dut.expect(r'\[DEBUG\] Notifying HTTP switch state change: Switch Index=4, New State=1', timeout=5)
    
    # 3. Wait for the button to be pressed again to turn Switch 4 OFF (State=0)
    print("\n[WAITING] Please trigger Button 5 again to turn the switch off...")
    dut.expect(r'\[DEBUG\] Received button event: Index=5, Event=0', timeout=60)
    dut.expect(r'\[DEBUG\] Executing switch command: Switch Index=4, Target State=0', timeout=5)