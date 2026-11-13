"""
@file       main.py
@brief      Standalone CLI for generating per-device SoftAP provisioning records.
@note       Run at manufacturing time. The password is written only to the --output file, never printed.
"""

import argparse
import sys
from pathlib import Path

from engine import DEFAULT_LENGTH, build_record, write_record


def parse_args():
    parser = argparse.ArgumentParser(
        description="Generate a per-device SoftAP password and write a JSON provisioning record."
    )
    parser.add_argument("--device-id", required=True, help="Device identifier")
    parser.add_argument(
        "--output", type=Path, required=True, help="Path of the JSON record to create"
    )
    parser.add_argument(
        "--length",
        type=int,
        default=DEFAULT_LENGTH,
        help=f"Password length, 8..63 (default: {DEFAULT_LENGTH})",
    )
    parser.add_argument(
        "--force", action="store_true", help="Overwrite an existing record"
    )
    args, extra_args = parser.parse_known_args()
    if extra_args:
        parser.error(f"unrecognized arguments: {' '.join(extra_args)}")
    return args


def main() -> int:
    args = parse_args()
    try:
        record = build_record(args.device_id, args.length)
        path = write_record(args.output, record, force=args.force)
    except (ValueError, OSError) as err:
        sys.stderr.write(f"Error: {err}\n")
        return 1

    print(
        f"[ap-password] Wrote provisioning record for device '{record['device_id']}': {path}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
