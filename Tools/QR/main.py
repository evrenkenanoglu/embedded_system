"""
@file       main.py
@brief      Standalone CLI engine: generic text-to-QR generator with a traceability log.
@note       Payload-agnostic. Knows nothing about Wi-Fi, SoftAP or the product.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import argparse
import os
import re
import sys
from pathlib import Path

import yaml

# Make the sibling qr_engine package importable when run as a plain script.
sys.path.insert(0, str(Path(__file__).resolve().parent))

from qr_engine import TOOL_NAME, TOOL_VERSION, QrToolError  # noqa: E402
from qr_engine.render import ECC_LEVELS, build_qr, save_symbol  # noqa: E402
from qr_engine.traceability import append_entry, build_entry  # noqa: E402

DEFAULTS = {"error_correction": "M", "scale": 10, "border": 4}
DEVICE_ID_RE = re.compile(r"[A-Za-z0-9._:\-]{1,64}")


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description="Generic QR code generator with an append-only traceability log.",
    )
    parser.add_argument(
        "config",
        nargs="?",
        type=Path,
        default=None,
        help="Optional YAML with error_correction/scale/border defaults (CLI flags override)",
    )
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument(
        "--payload",
        help="Payload text. Visible in the process list; prefer --stdin or --payload-env for secrets",
    )
    source.add_argument(
        "--stdin",
        action="store_true",
        help="Read the payload from stdin (one trailing newline is removed)",
    )
    source.add_argument(
        "--payload-env",
        metavar="VAR",
        help="Read the payload from environment variable VAR",
    )
    parser.add_argument(
        "--device-id", required=True, help="Device identifier, [A-Za-z0-9._:-], max 64"
    )
    parser.add_argument(
        "--output",
        type=Path,
        required=True,
        help="PNG output path (must not exist unless --force)",
    )
    parser.add_argument(
        "--svg", type=Path, default=None, help="Optional SVG output path"
    )
    parser.add_argument(
        "--log",
        type=Path,
        required=True,
        help="Append-only JSON Lines traceability log (required; never stores the payload)",
    )
    parser.add_argument(
        "--ecc",
        type=str.upper,
        choices=ECC_LEVELS,
        default=None,
        help="Error correction: L, M, Q, H",
    )
    parser.add_argument(
        "--scale", type=int, default=None, help="Pixels per module (1-100)"
    )
    parser.add_argument(
        "--border", type=int, default=None, help="Quiet-zone width in modules (0-100)"
    )
    parser.add_argument(
        "--force", action="store_true", help="Overwrite existing output files"
    )
    # parse_known_args avoids greedy-parsing bugs; unknown flags are reported, not silently dropped.
    return parser.parse_known_args(argv)


def load_defaults(config_path):
    values = dict(DEFAULTS)
    if config_path is None:
        return values
    if not config_path.is_file():
        raise QrToolError(f"config file not found: {config_path}")
    with open(config_path, "r", encoding="utf-8") as fh:
        data = yaml.safe_load(fh) or {}
    section = data.get("qr", data) if isinstance(data, dict) else {}
    for key in DEFAULTS:
        if key in section:
            values[key] = section[key]
    return values


def read_payload(args) -> str:
    if args.stdin:
        try:
            text = sys.stdin.buffer.read().decode("utf-8")
        except UnicodeDecodeError as exc:
            raise QrToolError("stdin payload is not valid UTF-8") from exc
        if text.endswith("\r\n"):
            text = text[:-2]
        elif text.endswith("\n"):
            text = text[:-1]
    elif args.payload_env is not None:
        if args.payload_env not in os.environ:
            raise QrToolError(f"environment variable {args.payload_env} is not set")
        text = os.environ[args.payload_env]
    else:
        text = args.payload
    if not text:
        raise QrToolError("payload is empty")
    return text


def _require_parent_dir(path: Path, label: str) -> None:
    if not path.parent.is_dir():
        raise QrToolError(f"{label} directory does not exist: {path.parent}")


def run(argv) -> int:
    args, extras = parse_args(argv)
    if extras:
        sys.stderr.write(
            f"warning: ignoring unrecognized arguments: {' '.join(extras)}\n"
        )

    defaults = load_defaults(args.config)
    ecc = (args.ecc or str(defaults["error_correction"])).upper()
    if ecc not in ECC_LEVELS:
        raise QrToolError(f"error_correction must be one of {', '.join(ECC_LEVELS)}")
    scale = int(args.scale if args.scale is not None else defaults["scale"])
    border = int(args.border if args.border is not None else defaults["border"])
    if not 1 <= scale <= 100:
        raise QrToolError("scale must be between 1 and 100")
    if not 0 <= border <= 100:
        raise QrToolError("border must be between 0 and 100")
    if not DEVICE_ID_RE.fullmatch(args.device_id):
        raise QrToolError(
            "device-id must match [A-Za-z0-9._:-] and be at most 64 characters"
        )

    outputs_paths = [args.output] + ([args.svg] if args.svg else [])
    if len({p.resolve() for p in outputs_paths}) != len(outputs_paths):
        raise QrToolError("output paths must be distinct")
    if args.log.resolve() in {p.resolve() for p in outputs_paths}:
        raise QrToolError("the log path must not be an output path")
    _require_parent_dir(args.output, "output")
    if args.svg:
        _require_parent_dir(args.svg, "svg output")
    _require_parent_dir(args.log, "log")

    payload = read_payload(args)

    # Creating the log before rendering fails fast if it is not writable.
    with open(args.log, "a", encoding="utf-8"):
        pass

    qr = build_qr(payload, ecc)
    save_symbol(qr, args.output, "png", scale, border, args.force)
    outputs = {"png": args.output}
    if args.svg:
        save_symbol(qr, args.svg, "svg", scale, border, args.force)
        outputs["svg"] = args.svg

    entry = build_entry(
        device_id=args.device_id,
        tool_version=TOOL_VERSION,
        payload=payload,
        ecc=ecc,
        scale=scale,
        border=border,
        outputs=outputs,
    )
    append_entry(args.log, entry)

    print(
        f"[{TOOL_NAME}] device={args.device_id} ecc={ecc} scale={scale} border={border}"
    )
    for fmt, path in outputs.items():
        digest = next(o["sha256"] for o in entry["outputs"] if o["format"] == fmt)
        print(f"[{TOOL_NAME}] {fmt}: {path} sha256={digest}")
    print(f"[{TOOL_NAME}] traceability entry appended to {args.log}")
    return 0


def main() -> None:
    try:
        sys.exit(run(sys.argv[1:]))
    except QrToolError as exc:
        sys.stderr.write(f"Error: {exc}\n")
        sys.exit(1)
    except OSError as exc:
        sys.stderr.write(f"Error: I/O failure: {exc.strerror} ({exc.filename})\n")
        sys.exit(1)


if __name__ == "__main__":
    main()
