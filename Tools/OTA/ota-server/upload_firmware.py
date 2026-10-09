#!/usr/bin/env python3
"""
@file       upload_firmware.py
@brief      SSoT-driven CLI utility to upload signed firmware to the OTA server API.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import argparse
import sys
from pathlib import Path
from typing import Optional, Dict, Any

try:
    import requests
    import urllib3
    import yaml
except ImportError:
    print(
        "ERROR: Missing dependencies. Run: pip install requests pyyaml", file=sys.stderr
    )
    sys.exit(1)


def load_yaml_config(config_path: Path) -> Dict[str, Any]:
    """Loads configuration file safely."""
    if not config_path.exists():
        return {}
    with open(config_path, "r", encoding="utf-8") as f:
        return yaml.safe_load(f) or {}


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="SSoT-driven CLI utility to upload firmware payloads to the OTA server.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument(
        "--config",
        "-C",
        type=Path,
        default=None,
        help="Path to SSoT config file (config_project.yaml or resolved config)",
    )
    parser.add_argument(
        "-u",
        "--url",
        default=None,
        help="Destination upload URL endpoint (default: derived from config gateway_url)",
    )
    parser.add_argument(
        "-f",
        "--file",
        default=None,
        help="Local path to .bin file (default: derived from build_dir/binary_name)",
    )
    parser.add_argument(
        "-d",
        "--hw",
        default=None,
        help="Target hardware identifier (default: derived from config hardware.model)",
    )
    parser.add_argument(
        "-v",
        "--version",
        default=None,
        help="Firmware semver string (default: derived from config project.version)",
    )
    parser.add_argument(
        "-n",
        "--notes",
        default="Automated release pipeline upload.",
        help="Descriptive change log or release notes",
    )
    parser.add_argument(
        "-c",
        "--channel",
        default="stable",
        help="Deployment channel target (stable, beta, testing, development)",
    )
    parser.add_argument(
        "--hsvn",
        type=int,
        default=None,
        help="Hardware Security Version Number (default: derived from config project.version_number)",
    )
    parser.add_argument(
        "--canary",
        type=int,
        default=100,
        help="Target canary rollout percentage (0 to 100)",
    )
    parser.add_argument(
        "-k",
        "--insecure",
        action="store_true",
        help="Bypass SSL certificate verification (recommended for local IP testing)",
    )
    parser.add_argument(
        "--ca-cert",
        type=Path,
        default=None,
        help="Path to Root CA certificate for HTTPS validation",
    )

    return parser.parse_args()


def upload_binary(args):
    cfg: Dict[str, Any] = {}
    if args.config:
        cfg = load_yaml_config(args.config.resolve())

    # 1. Resolve parameters (CLI explicit flag -> Config file -> Fallback)
    proj_cfg = cfg.get("project", {})
    hw_cfg = cfg.get("hardware", {})
    net_cfg = cfg.get("network", {})
    paths_cfg = cfg.get("paths", {})

    version = args.version or proj_cfg.get("version", "1.0.1")
    project_name = proj_cfg.get("name", "Embedded_IoT_BT_WIFI_Base_Project")
    hardware = args.hw or hw_cfg.get("model", "ESP32-C6-WROOM-1")
    hsvn = args.hsvn if args.hsvn is not None else proj_cfg.get("version_number", 1)

    # Resolve target upload URL
    gateway_url = net_cfg.get("gateway_url", "https://127.0.0.1:8443").rstrip("/")
    upload_url = args.url or f"{gateway_url}/upload"

    # Resolve local binary file
    if args.file:
        binary_path = Path(args.file).resolve()
    else:
        build_dir = Path(paths_cfg.get("build_dir", "build")).resolve()
        bin_name = proj_cfg.get("binary_name", f"{project_name}.bin")
        binary_path = build_dir / bin_name

    if not binary_path.exists():
        print(f"❌ ERROR: Firmware binary not found at: {binary_path}", file=sys.stderr)
        print(
            "   Run 'inv es.esp32.build' first to compile the binary.", file=sys.stderr
        )
        sys.exit(1)

    # 2. Assign immutable, version-tagged filename for server storage
    server_target_filename = f"{project_name}_{version}.bin"

    payload = {
        "hardware": hardware,
        "version": version,
        "release_notes": args.notes,
        "channel": args.channel,
        "hsvn": hsvn,
        "canary_percentage": args.canary,
    }

    # SSL validation setup
    ca_cert_path = args.ca_cert or Path(paths_cfg.get("certs_dir", "certs")) / "ca.crt"
    verify: Any = True
    if args.insecure:
        verify = False
        urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)
    elif ca_cert_path.exists():
        verify = str(ca_cert_path.resolve())

    file_size_mb = binary_path.stat().st_size / (1024 * 1024)

    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    print(f" ❯ 🚀 UPLOADING FIRMWARE RELEASE TO OTA SERVER")
    print(f"   Target URL   : {upload_url}")
    print(f"   Local Binary : {binary_path} ({file_size_mb:.2f} MB)")
    print(f"   Remote Name  : {server_target_filename}")
    print(
        f"   Metadata     : Version={version} | HW={hardware} | HSVN={hsvn} | Channel={args.channel}"
    )
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")

    try:
        with open(binary_path, "rb") as f:
            # Send file with its version-tagged filename so server stores it immutably
            files = {"file": (server_target_filename, f, "application/octet-stream")}
            response = requests.post(
                upload_url,
                data=payload,
                files=files,
                verify=verify,
                timeout=60.0,
                allow_redirects=True,
            )

        if response.status_code in [200, 303]:
            print("\n🎉 Upload Successful! Release registered and active on server.")
        else:
            print(
                f"\n❌ Upload Failed! Server status: {response.status_code}",
                file=sys.stderr,
            )
            print(f"   Server Response: {response.text}", file=sys.stderr)
            sys.exit(1)

    except requests.exceptions.SSLError as exc:
        print(f"\n❌ SSL Verification Error: {exc}", file=sys.stderr)
        print(
            "   Tip: Pass -k / --insecure for local self-signed testing.",
            file=sys.stderr,
        )
        sys.exit(1)
    except Exception as exc:
        print(
            f"\n❌ Connection Error: Failed to reach OTA server at {upload_url}: {exc}",
            file=sys.stderr,
        )
        sys.exit(1)


if __name__ == "__main__":
    upload_binary(parse_arguments())
