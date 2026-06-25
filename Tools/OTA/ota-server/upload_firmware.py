import argparse
import os
import sys
from pathlib import Path

try:
    import requests
    import urllib3
except ImportError:
    print("ERROR: This script requires the 'requests' library.")
    print("Run: pip install requests")
    sys.exit(1)


def parse_arguments():
    """Parses command-line arguments and returns the parsed options with default fallbacks."""
    parser = argparse.ArgumentParser(
        description="CLI utility to compile metadata and transfer firmware payloads to the OTA server.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    
    parser.add_argument(
        "-u", "--url", 
        default="https://localhost:8443/upload", 
        help="Destination upload URL endpoint"
    )
    
    parser.add_argument(
        "-f", "--file", 
        default="esp32_v1.0.3.bin", 
        help="Local file path to the binary (.bin) payload"
    )
    
    parser.add_argument(
        "-d", "--hw", 
        default="esp32-devkit-v1", 
        help="Compatibly matching target hardware identification signature"
    )
    
    parser.add_argument(
        "-v", "--version", 
        default="1.0.3", 
        help="Semantically ordered firmware version increment"
    )
    
    parser.add_argument(
        "-n", "--notes", 
        default="Command-line automated release upload.", 
        help="Descriptive change log or release notes"
    )
    
    parser.add_argument(
        "-k", "--insecure", 
        action="store_true", 
        help="Bypass SSL certificate authority validation checks (recommended for local testing)"
    )

    return parser.parse_args()


def upload_binary(args):
    """Executes the multipart HTTP POST transfer request to the destination server."""
    binary_path = Path(args.file)
    
    if not binary_path.exists() or not binary_path.is_file():
        print(f"ERROR: Firmware target file not found at: {binary_path.resolve()}")
        sys.exit(1)

    # Compile the POST form arguments
    payload = {
        "hardware": args.hw,
        "version": args.version,
        "release_notes": args.notes
    }

    # SSL configuration switch
    verify_ssl = not args.insecure

    if not verify_ssl:
        urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

    print(f"Uploading '{binary_path.name}' to {args.url}...")
    print(f"Metadata: Hardware={args.hw}, Version={args.version}, Insecure={args.insecure}")

    try:
        with open(binary_path, "rb") as f:
            files = {
                "file": (binary_path.name, f, "application/octet-stream")
            }
            
            response = requests.post(
                args.url, 
                data=payload, 
                files=files, 
                verify=verify_ssl,
                allow_redirects=True
            )

        if response.status_code in [200, 303]:
            print("Upload Successful! Manifest updated on the server.")
        else:
            print(f"Upload Failed. Server returned Status Code: {response.status_code}")
            print(f"Response: {response.text}")

    except Exception as e:
        print(f"Connection Error: Could not reach the OTA server: {e}")
        sys.exit(1)


if __name__ == "__main__":
    parsed_args = parse_arguments()
    upload_binary(parsed_args)