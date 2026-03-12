import config
import re
import subprocess
import qrcode


def run_command(cmd):
    """Executes a shell command and returns the output."""
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        return result.stdout.strip()
    except subprocess.CalledProcessError as e:
        print(f"Error executing command: {e.stderr}")
        return None

def extract_payload(output):
    """
    Extracts the actual code (MT:... or digits) from chip-tool's 
    verbose log output using regex.
    """
    # Regex for QR Code (Starts with MT:)
    qr_match = re.search(r'MT:[A-Z0-9.-]+', output)
    if qr_match:
        return qr_match.group(0)
    
    # Regex for Manual Code (Digits only, usually 11 or 21)
    manual_match = re.search(r'\d{11,21}', output)
    if manual_match:
        return manual_match.group(0)
    
    return "Not Found"

def printQRCodeWithWebsite(qr_code_payload):
    website = "https://project-chip.github.io/connectedhomeip/qrcode.html?data="
    print(f"QR CODE: {website}{qr_code_payload}")    

def main():
    print(f"--- Generating Matter Codes for PID {config.PRODUCT_ID} ---")

    # 1. Generate QR Code
    qr_cmd = [
        config.CHIP_TOOL_PATH, "payload", "generate-qrcode",
        "--discriminator", str(config.DISCRIMINATOR),
        "--setup-pin-code", str(config.SETUP_PIN_CODE),
        "--vendor-id", str(config.VENDOR_ID),
        "--product-id", str(config.PRODUCT_ID),
        "--version", str(config.VERSION),
        "--commissioning-mode", "0",
        "--rendezvous", str(config.RENDEZVOUS)
    ]
    
    qr_output = run_command(qr_cmd)
    qr_code = extract_payload(qr_output)
    print(f"QR Code Payload:    {qr_code}")

    # 2. Generate Short Manual Code (11-digit)
    short_cmd = [
        config.CHIP_TOOL_PATH, "payload", "generate-manualcode",
        "--discriminator", str(config.DISCRIMINATOR),
        "--setup-pin-code", str(config.SETUP_PIN_CODE),
        "--version", str(config.VERSION),
        "--commissioning-mode", "0"
    ]
    
    short_output = run_command(short_cmd)
    short_code = extract_payload(short_output)
    print(f"Manual Code (11d):  {short_code}")

    # 3. Generate Long Manual Code (21-digit)
    long_cmd = [
        config.CHIP_TOOL_PATH, "payload", "generate-manualcode",
        "--discriminator", str(config.DISCRIMINATOR),
        "--setup-pin-code", str(config.SETUP_PIN_CODE),
        "--vendor-id", str(config.VENDOR_ID),
        "--product-id", str(config.PRODUCT_ID),
        "--version", str(config.VERSION),
        "--commissioning-mode", "1"
    ]
    
    long_output = run_command(long_cmd)
    long_code = extract_payload(long_output)
    print(f"Manual Code (21d):  {long_code}")

    printQRCodeWithWebsite(qr_code)
    img = qrcode.make(qr_code)
    img.save("qr_code.png")
    

if __name__ == "__main__":
    main()