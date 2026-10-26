#!/usr/bin/env python3
"""
@file       run.py
@brief      Automated OTA Dual-Phase test orchestrator.
            Manages catalog purging, version staging, baseline flashing, update publishing, and verification.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import argparse
import re
import shlex
import subprocess
import sys
from pathlib import Path
from typing import Optional


def find_workspace_root() -> Path:
    """Traverses upward from script directory to find workspace root with configs/."""
    curr = Path(__file__).resolve().parent
    while curr != curr.parent:
        if (curr / "configs" / "config_project.yaml").exists():
            return curr
        curr = curr.parent
    return Path.cwd().resolve()


WORKSPACE_ROOT = find_workspace_root()
CONFIG_PROJECT_PATH = WORKSPACE_ROOT / "configs" / "config_project.yaml"


def update_ssot_version(config_path: Path, version: str, version_number: int) -> None:
    """Updates project.version and project.version_number in configs/config_project.yaml without stripping comments."""
    if not config_path.exists():
        raise FileNotFoundError(f"Master SSoT config not found: {config_path}")

    content = config_path.read_text(encoding="utf-8")

    # Update project version string
    content = re.sub(
        r"(^\s*version:\s*)['\"]?[^\s'\"]+['\"]?",
        rf"\g<1>{version}",
        content,
        flags=re.MULTILINE,
    )
    # Update project version integer
    content = re.sub(
        r"(^\s*version_number:\s*)\d+",
        rf"\g<1>{version_number}",
        content,
        flags=re.MULTILINE,
    )

    config_path.write_text(content, encoding="utf-8")
    print(
        f"[*] [SSOT] Updated {config_path.name} -> Version: {version} (Number: {version_number})"
    )


def run_command(cmd: str, check: bool = True) -> int:
    """Runs a shell command inside the workspace directory."""
    print(f"\n[RUN] {cmd}")
    res = subprocess.run(cmd, shell=True, cwd=str(WORKSPACE_ROOT))
    if check and res.returncode != 0:
        print(
            f"[FATAL ERROR] Command failed with exit code {res.returncode}: {cmd}",
            file=sys.stderr,
        )
        sys.exit(res.returncode)
    return res.returncode


def cmd_clean() -> None:
    """Purges release-catalog binaries, patches, and manifest.json."""
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    print(" ❯ 🧹 OTA-TESTER: PURGING RELEASE CATALOG")
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    run_command("inv tools.release-catalog.clean")


def cmd_flash_chain(monitor: bool = True) -> None:
    """Executes the complete baseline build, NVS provisioning, and flashing command chain."""
    chain = [
        "inv es.build-config.all",
        "inv es.platform.build",
        "inv es.crypto.provision-nvs",
        "inv es.platform.flash",
        "inv es.crypto.flash-nvs",
    ]
    if monitor:
        chain.append("inv es.platform.monitor")

    run_command(" && ".join(chain))


def cmd_baseline(
    version: str = "1.0.0",
    version_number: int = 1,
    monitor: bool = False,
    clean_catalog: bool = True,
) -> None:
    """Purges catalog, sets version to baseline, builds, provisions, flashes hardware, and catalog baseline."""
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    print(f" ❯ 📦 OTA-TESTER: STAGING & FLASHING BASELINE FIRMWARE (v{version})")
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")

    # 1. Purge stale catalog artifacts
    if clean_catalog:
        cmd_clean()

    # 2. Synchronize SSoT config version
    update_ssot_version(CONFIG_PROJECT_PATH, version, version_number)

    # 3. Build and Flash Baseline to Hardware
    cmd_flash_chain(monitor=False)

    # 4. Package Baseline into Release Catalog
    print("\n[*] [BASELINE] Packaging and signing baseline into release catalog...")
    run_command("inv es.crypto.provision-sign")
    run_command("inv tools.release-catalog.reconcile")
    run_command("inv es.util.sync-ota-windows")

    print(f"\n✅ Baseline v{version} flashed to board and cataloged successfully!")

    if monitor:
        run_command("inv es.platform.monitor")


def cmd_update(version: str = "1.0.1", version_number: int = 2) -> None:
    """Sets version to target update, compiles, signs, creates 1-hop delta, and syncs without flashing."""
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    print(f" ❯ 🚀 OTA-TESTER: PACKAGING & DEPLOYING TARGET UPDATE (v{version})")
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")

    update_ssot_version(CONFIG_PROJECT_PATH, version, version_number)

    # 1. Re-generate SSoT Artifacts & Build Version
    run_command("inv es.build-config.all")

    # 2. Run Canonical OTA Release Pipeline (Build -> Sign -> 1-Hop Diff -> Sync)
    run_command("inv es.ota.release")

    print(
        f"\n✅ Target update v{version} published! Device running baseline will now receive this update over Wi-Fi."
    )


def cmd_e2e(
    base_ver: str = "1.0.0",
    base_num: int = 1,
    target_ver: str = "1.0.1",
    target_num: int = 2,
    clean_catalog: bool = True,
) -> None:
    """Executes full end-to-end OTA cycle: Purge catalog -> Flash baseline -> Publish update -> Monitor without reset."""
    print("======================================================================")
    print(" 🔄 EXECUTING FULL AUTOMATED END-TO-END OTA LIFECYCLE TEST")
    print("======================================================================")

    # Phase 1: Purge Catalog, Setup, and Flash Baseline
    cmd_baseline(
        version=base_ver,
        version_number=base_num,
        monitor=False,
        clean_catalog=clean_catalog,
    )

    # Phase 2: Stage and Publish New Update
    cmd_update(version=target_ver, version_number=target_num)

    # Phase 3: Monitor Live OTA Handshake & Flash Write
    print("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    print(" ❯ 🖥️ ATTACHING MONITOR (Observing Live OTA Download & Provisional Boot)")
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    run_command('inv es.platform.monitor --opts="--no-reset"')


def main() -> int:
    parser = argparse.ArgumentParser(
        description="OTA Automated Testing & Staging Tool."
    )
    subparsers = parser.add_subparsers(dest="command", help="Target testing action")

    # clean
    subparsers.add_parser(
        "clean", help="Purge release-catalog binaries, patches, and manifest.json."
    )

    # baseline
    p_base = subparsers.add_parser(
        "baseline", help="Purge catalog, configure, build, flash, and catalog baseline."
    )
    p_base.add_argument(
        "--version", type=str, default="1.0.0", help="Baseline SemVer (default: 1.0.0)"
    )
    p_base.add_argument(
        "--version-number", type=int, default=1, help="Baseline integer (default: 1)"
    )
    p_base.add_argument(
        "--monitor", action="store_true", help="Open serial monitor after flashing"
    )
    p_base.add_argument(
        "--no-clean",
        action="store_true",
        help="Skip catalog cleaning before baseline setup",
    )

    # update
    p_up = subparsers.add_parser(
        "update", help="Stage, compile, sign, diff, and publish target update."
    )
    p_up.add_argument(
        "--version",
        type=str,
        default="1.0.1",
        help="Target update SemVer (default: 1.0.1)",
    )
    p_up.add_argument(
        "--version-number",
        type=int,
        default=2,
        help="Target update integer (default: 2)",
    )

    # flash-chain
    p_chain = subparsers.add_parser(
        "flash-chain", help="Run the full build, provision, flash, and monitor chain."
    )
    p_chain.add_argument(
        "--no-monitor", action="store_true", help="Do not launch serial monitor"
    )

    # e2e
    p_e2e = subparsers.add_parser(
        "e2e",
        help="Run full lifecycle: clean -> baseline flash -> update release -> monitor.",
    )
    p_e2e.add_argument("--base-ver", type=str, default="1.0.0", help="Baseline SemVer")
    p_e2e.add_argument(
        "--base-num", type=int, default=1, help="Baseline version number"
    )
    p_e2e.add_argument("--target-ver", type=str, default="1.0.1", help="Target SemVer")
    p_e2e.add_argument(
        "--target-num", type=int, default=2, help="Target version number"
    )
    p_e2e.add_argument(
        "--no-clean",
        action="store_true",
        help="Skip catalog cleaning before running test",
    )

    args = parser.parse_args()

    if args.command == "clean":
        cmd_clean()
    elif args.command == "baseline":
        cmd_baseline(
            args.version,
            args.version_number,
            args.monitor,
            clean_catalog=not args.no_clean,
        )
    elif args.command == "update":
        cmd_update(args.version, args.version_number)
    elif args.command == "flash-chain":
        cmd_flash_chain(monitor=not args.no_monitor)
    elif args.command == "e2e":
        cmd_e2e(
            args.base_ver,
            args.base_num,
            args.target_ver,
            args.target_num,
            clean_catalog=not args.no_clean,
        )
    else:
        parser.print_help()
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
