"""
@file       private_ca.py
@brief      Invoke tasks for the offline private CA (root CA init, per-device HTTPS server certificate signing,
            and signing of device-generated CSRs).
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import shlex
import sys
from pathlib import Path
from invoke import Context, task
from core import CONFIG

PRIVATE_CA_SCRIPT = (
    Path(__file__).resolve().parent.parent
    / "Tools"
    / "HW_SECURITY"
    / "private_ca"
    / "private_ca.py"
)


def _run_tool(c: Context, args: list, dry_run: bool) -> None:
    cmd = [sys.executable, str(PRIVATE_CA_SCRIPT), *args]
    c.run(shlex.join(cmd), dry=dry_run, pty=(sys.platform != "win32"))


def _split_csv(value: str) -> list:
    return [item.strip() for item in value.split(",") if item.strip()]


@task(
    help={
        "out_dir": "Output directory for root_ca.key and root_ca.pem (must be outside the repository)",
        "common_name": "Root CA common name",
        "days": "Root CA validity in days",
        "passphrase_env": "Name of the environment variable holding the root key passphrase (value is never passed on the command line)",
        "force": "Replace an existing root CA",
        "dry_run": "Print command line without executing",
    }
)
def init_ca(
    c: Context,
    out_dir: str = "",
    common_name: str = "",
    days: str = "",
    passphrase_env: str = "",
    force: bool = False,
    dry_run: bool = False,
) -> None:
    """Create the offline private root CA (root_ca.key + root_ca.pem) outside the repository."""
    if not out_dir.strip():
        raise ValueError("--out-dir is required and must point outside the repository.")
    args = ["init-ca", "--out-dir", out_dir.strip()]
    if common_name.strip():
        args += ["--common-name", common_name.strip()]
    if days.strip():
        args += ["--days", days.strip()]
    if passphrase_env.strip():
        args += ["--passphrase-env", passphrase_env.strip()]
    if force:
        args.append("--force")
    _run_tool(c, args, dry_run)


@task(
    help={
        "ca_dir": "Directory containing root_ca.key and root_ca.pem created by init-ca",
        "out_dir": "Output directory for device_server.key and device_server.pem (must be outside the repository)",
        "san_ip": "Comma-separated IP SANs (default: security.device_cert.san_ip in the SSoT)",
        "san_dns": "Comma-separated DNS SANs (default: security.device_cert.san_dns in the SSoT)",
        "common_name": "Subject CN (defaults to the first SAN)",
        "days": "Device certificate validity in days (default: security.device_cert.validity_days)",
        "passphrase_env": "Name of the environment variable holding the root key passphrase",
        "force": "Replace an existing device identity in out_dir",
        "dry_run": "Print command line without executing",
    }
)
def sign_device(
    c: Context,
    ca_dir: str = "",
    out_dir: str = "",
    san_ip: str = "",
    san_dns: str = "",
    common_name: str = "",
    days: str = "",
    passphrase_env: str = "",
    force: bool = False,
    dry_run: bool = False,
) -> None:
    """Sign a per-device HTTPS server certificate (SAN IP + DNS) with the offline root CA."""
    if not ca_dir.strip():
        raise ValueError("--ca-dir is required.")
    if not out_dir.strip():
        raise ValueError("--out-dir is required and must point outside the repository.")
    cfg = CONFIG.security.device_cert
    args = ["sign-device", "--ca-dir", ca_dir.strip(), "--out-dir", out_dir.strip()]
    for ip in _split_csv(san_ip) or [str(item) for item in cfg.san_ip]:
        args += ["--san-ip", ip]
    for dns_name in _split_csv(san_dns) or [str(item) for item in cfg.san_dns]:
        args += ["--san-dns", dns_name]
    if common_name.strip():
        args += ["--common-name", common_name.strip()]
    args += ["--days", days.strip() or str(cfg.validity_days)]
    if passphrase_env.strip():
        args += ["--passphrase-env", passphrase_env.strip()]
    if force:
        args.append("--force")
    _run_tool(c, args, dry_run)


@task(
    help={
        "ca_dir": "Directory containing root_ca.key and root_ca.pem created by init-ca",
        "csr": "PKCS#10 CSR PEM file saved from the device serial output (BEGIN/END CERTIFICATE REQUEST)",
        "out": "Output device certificate PEM path (must be outside the repository)",
        "device_id": "Device identifier recorded in the traceability log (required)",
        "san_ip": "Comma-separated IP SANs (default: security.device_cert.san_ip in the SSoT)",
        "san_dns": "Comma-separated DNS SANs (default: security.device_cert.san_dns in the SSoT)",
        "common_name": "Subject CN (defaults to the first SAN)",
        "days": "Device certificate validity in days (default: security.device_cert.validity_days)",
        "log": "JSON Lines traceability log (default: security.device_cert.traceability_log)",
        "passphrase_env": "Name of the environment variable holding the root key passphrase",
        "force": "Replace an existing file at out",
        "dry_run": "Print command line without executing",
    }
)
def sign_csr(
    c: Context,
    ca_dir: str = "",
    csr: str = "",
    out: str = "",
    device_id: str = "",
    san_ip: str = "",
    san_dns: str = "",
    common_name: str = "",
    days: str = "",
    log: str = "",
    passphrase_env: str = "",
    force: bool = False,
    dry_run: bool = False,
) -> None:
    """Sign a device-held CSR with the offline root CA. The device key never leaves the device."""
    if not ca_dir.strip():
        raise ValueError("--ca-dir is required.")
    if not csr.strip():
        raise ValueError("--csr is required.")
    if not out.strip():
        raise ValueError("--out is required and must point outside the repository.")
    if not device_id.strip():
        raise ValueError("--device-id is required.")

    cfg = CONFIG.security.device_cert
    workspace_root = Path(CONFIG.paths.workspace_dir).resolve()
    log_path = Path(log.strip() or str(cfg.traceability_log))
    if not log_path.is_absolute():
        log_path = (workspace_root / log_path).resolve()

    args = [
        "sign-csr",
        "--ca-dir",
        ca_dir.strip(),
        "--csr",
        csr.strip(),
        "--out",
        out.strip(),
    ]
    args += ["--device-id", device_id.strip(), "--log", str(log_path)]
    for ip in _split_csv(san_ip) or [str(item) for item in cfg.san_ip]:
        args += ["--san-ip", ip]
    for dns_name in _split_csv(san_dns) or [str(item) for item in cfg.san_dns]:
        args += ["--san-dns", dns_name]
    args += ["--days", days.strip() or str(cfg.validity_days)]
    if common_name.strip():
        args += ["--common-name", common_name.strip()]
    if passphrase_env.strip():
        args += ["--passphrase-env", passphrase_env.strip()]
    if force:
        args.append("--force")
    _run_tool(c, args, dry_run)
