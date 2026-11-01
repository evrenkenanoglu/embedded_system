import argparse
import os
import re
import subprocess

import qrcode

import config


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
    qr_match = re.search(r"MT:[A-Z0-9.-]+", output)
    if qr_match:
        return qr_match.group(0)

    # Regex for Manual Code (Digits only, usually 11 or 21)
    manual_match = re.search(r"\d{11,21}", output)
    if manual_match:
        return manual_match.group(0)

    return "Not Found"


def printQRCodeWithWebsite(qr_code_payload):
    website = "https://project-chip.github.io/connectedhomeip/qrcode.html?data="
    print(f"QR CODE: {website}{qr_code_payload}")


def _integer(value):
    try:
        parsed = int(value, 16 if value.lower().startswith("0x") else 10)
    except ValueError as error:
        raise argparse.ArgumentTypeError("must be an integer") from error
    if parsed < 0:
        raise argparse.ArgumentTypeError("must not be negative")
    return parsed


def _discriminator(value):
    parsed = _integer(value)
    if parsed > 0x0FFF:
        raise argparse.ArgumentTypeError("must fit in 12 bits (0..4095)")
    return parsed


def _setup_pin_code(value):
    parsed = _integer(value)
    invalid_codes = {digit * 11111111 for digit in range(1, 10)} | {
        12345678,
        87654321,
        20202021,
    }
    if parsed == 0 or parsed > 99999998 or parsed in invalid_codes:
        raise argparse.ArgumentTypeError("must be a valid Matter setup passcode")
    return parsed


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Generate per-device Matter onboarding codes."
    )
    parser.add_argument(
        "--discriminator",
        type=_discriminator,
        default=os.environ.get("MATTER_DISCRIMINATOR"),
        required=os.environ.get("MATTER_DISCRIMINATOR") is None,
        help="per-device 12-bit discriminator (or MATTER_DISCRIMINATOR)",
    )
    parser.add_argument(
        "--setup-pin-code",
        type=_setup_pin_code,
        default=os.environ.get("MATTER_SETUP_PIN_CODE"),
        required=os.environ.get("MATTER_SETUP_PIN_CODE") is None,
        help="per-device Matter setup passcode (or MATTER_SETUP_PIN_CODE)",
    )
    args = parser.parse_args(argv)

    print(f"--- Generating Matter Codes for PID {config.PRODUCT_ID} ---")

    # 1. Generate QR Code
    qr_cmd = [
        config.CHIP_TOOL_PATH,
        "payload",
        "generate-qrcode",
        "--discriminator",
        str(args.discriminator),
        "--setup-pin-code",
        str(args.setup_pin_code),
        "--vendor-id",
        str(config.VENDOR_ID),
        "--product-id",
        str(config.PRODUCT_ID),
        "--version",
        str(config.VERSION),
        "--commissioning-mode",
        "0",
        "--rendezvous",
        str(config.RENDEZVOUS),
    ]

    qr_output = run_command(qr_cmd)
    qr_code = extract_payload(qr_output)
    print(f"QR Code Payload:    {qr_code}")

    # 2. Generate Short Manual Code (11-digit)
    short_cmd = [
        config.CHIP_TOOL_PATH,
        "payload",
        "generate-manualcode",
        "--discriminator",
        str(args.discriminator),
        "--setup-pin-code",
        str(args.setup_pin_code),
        "--version",
        str(config.VERSION),
        "--commissioning-mode",
        "0",
    ]

    short_output = run_command(short_cmd)
    short_code = extract_payload(short_output)
    print(f"Manual Code (11d):  {short_code}")

    # 3. Generate Long Manual Code (21-digit)
    long_cmd = [
        config.CHIP_TOOL_PATH,
        "payload",
        "generate-manualcode",
        "--discriminator",
        str(args.discriminator),
        "--setup-pin-code",
        str(args.setup_pin_code),
        "--vendor-id",
        str(config.VENDOR_ID),
        "--product-id",
        str(config.PRODUCT_ID),
        "--version",
        str(config.VERSION),
        "--commissioning-mode",
        "1",
    ]

    long_output = run_command(long_cmd)
    long_code = extract_payload(long_output)
    print(f"Manual Code (21d):  {long_code}")

    printQRCodeWithWebsite(qr_code)
    img = qrcode.make(qr_code)
    img.save("qr_code.png")


if __name__ == "__main__":
    main()
