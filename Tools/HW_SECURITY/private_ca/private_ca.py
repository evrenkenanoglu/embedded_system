#!/usr/bin/env python3
"""
@file       private_ca.py
@brief      Standalone private CA: offline root CA creation and per-device HTTPS server certificate signing.

Subcommands:
  init-ca      Create root_ca.key (EC P-256) and root_ca.pem (self-signed CA certificate) in --out-dir.
  sign-device  Create device_server.key and device_server.pem for one device, with SAN IP/DNS entries,
               signed by the root CA found in --ca-dir.
  sign-csr     Sign a device-generated PKCS#10 CSR (EC P-256) with the root CA. The CSR signature must
               verify; SAN set and validity come from the caller (the inv task reads them from the SSoT).
               Appends one JSON Lines traceability record (hashes only, never PEM or key material).

Handoff contract:
  - root_ca.pem is the public trust anchor. Install it once on each phone/browser as a trusted root.
  - root_ca.key must stay offline. Keep the root directory outside any git worktree and back it up
    encrypted (use --passphrase-env to encrypt the key at rest).
  - device_server.pem / device_server.key are the device TLS identity for HttpsServer. They are written
    as PEM. sign-csr writes a device certificate for a device-held key; the NVS entry is produced by the
    provisioning NVS template (security.device_cert in the SSoT).
  - Passphrases are read from the environment variable named by --passphrase-env, never from argv.
  - Refuses output directories inside any git working tree so key material cannot be committed by accident.
"""

import argparse
import datetime
import hashlib
import ipaddress
import json
import os
import re
import sys
from pathlib import Path
from typing import List, Optional

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.x509.oid import ExtendedKeyUsageOID, NameOID

TOOL_NAME = "private-ca"
TOOL_VERSION = "1.1.0"
ROOT_KEY_FILE = "root_ca.key"
ROOT_CERT_FILE = "root_ca.pem"
DEVICE_KEY_FILE = "device_server.key"
DEVICE_CERT_FILE = "device_server.pem"
DEFAULT_CA_DAYS = 3650
DEFAULT_DEVICE_DAYS = 825
DEFAULT_CA_COMMON_NAME = "Embedded IoT Private Root CA"
DNS_NAME_PATTERN = re.compile(
    r"^[A-Za-z0-9*]([A-Za-z0-9-]{0,62}[A-Za-z0-9])?(\.[A-Za-z0-9]([A-Za-z0-9-]{0,62}[A-Za-z0-9])?)*$"
)
DEVICE_ID_PATTERN = re.compile(r"^[A-Za-z0-9._:\-]{1,64}$")


def _now() -> datetime.datetime:
    return datetime.datetime.now(datetime.timezone.utc)


def _fail(message: str) -> None:
    raise SystemExit(f"[ERROR] {message}")


def _assert_outside_repo(directory: Path) -> None:
    """Refuses directories that sit inside any git working tree (a .git file or directory in an ancestor)."""
    for candidate in [directory, *directory.parents]:
        if (candidate / ".git").exists():
            _fail(
                f"Refusing to write key material inside a git worktree ({candidate}). Use a directory outside the repository."
            )


def _prepare_out_dir(directory: Path) -> Path:
    resolved = directory.expanduser().resolve()
    _assert_outside_repo(resolved)
    resolved.mkdir(parents=True, exist_ok=True)
    os.chmod(resolved, 0o700)
    return resolved


def _passphrase(env_name: str) -> Optional[bytes]:
    if not env_name:
        return None
    value = os.environ.get(env_name, "")
    if not value:
        _fail(f"Environment variable '{env_name}' is unset or empty.")
    return value.encode("utf-8")


def _write_secret(path: Path, data: bytes, force: bool) -> None:
    if path.exists() and not force:
        _fail(f"Refusing to overwrite {path} (use --force).")
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    with os.fdopen(fd, "wb") as handle:
        handle.write(data)
    os.chmod(path, 0o600)


def _write_public(path: Path, data: bytes, force: bool) -> None:
    if path.exists() and not force:
        _fail(f"Refusing to overwrite {path} (use --force).")
    path.write_bytes(data)
    os.chmod(path, 0o644)


def _serialize_key(
    key: ec.EllipticCurvePrivateKey, passphrase: Optional[bytes]
) -> bytes:
    encryption = (
        serialization.BestAvailableEncryption(passphrase)
        if passphrase
        else serialization.NoEncryption()
    )
    return key.private_bytes(
        serialization.Encoding.PEM, serialization.PrivateFormat.PKCS8, encryption
    )


def _load_ca(ca_dir: Path, passphrase: Optional[bytes]):
    key_path = ca_dir / ROOT_KEY_FILE
    cert_path = ca_dir / ROOT_CERT_FILE
    if not key_path.is_file() or not cert_path.is_file():
        _fail(
            f"Root CA not found in {ca_dir} (expected {ROOT_KEY_FILE} and {ROOT_CERT_FILE}). Run init-ca first."
        )
    try:
        ca_key = serialization.load_pem_private_key(
            key_path.read_bytes(), password=passphrase
        )
    except (TypeError, ValueError) as error:
        _fail(
            f"Cannot load {key_path} (wrong or missing --passphrase-env value?): {error}"
        )
    ca_cert = x509.load_pem_x509_certificate(cert_path.read_bytes())

    basic = ca_cert.extensions.get_extension_for_class(x509.BasicConstraints).value
    if not basic.ca:
        _fail(f"{cert_path} is not a CA certificate.")
    spki = (serialization.Encoding.DER, serialization.PublicFormat.SubjectPublicKeyInfo)
    if ca_cert.public_key().public_bytes(*spki) != ca_key.public_key().public_bytes(
        *spki
    ):
        _fail(f"{key_path} does not match the public key in {cert_path}.")
    return ca_key, ca_cert


def cmd_init_ca(args: argparse.Namespace) -> int:
    if args.days <= 0:
        _fail("--days must be positive.")
    out_dir = _prepare_out_dir(args.out_dir)
    key_path = out_dir / ROOT_KEY_FILE
    cert_path = out_dir / ROOT_CERT_FILE
    if key_path.exists() and not args.force:
        _fail(
            f"Root CA already exists in {out_dir}. Regenerating it invalidates every phone that trusts it; use --force only deliberately."
        )

    passphrase = _passphrase(args.passphrase_env)
    key = ec.generate_private_key(ec.SECP256R1())
    name = x509.Name(
        [
            x509.NameAttribute(NameOID.COMMON_NAME, args.common_name),
            x509.NameAttribute(NameOID.ORGANIZATION_NAME, "Embedded IoT"),
        ]
    )
    now = _now()
    cert = (
        x509.CertificateBuilder()
        .subject_name(name)
        .issuer_name(name)
        .public_key(key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(now - datetime.timedelta(minutes=5))
        .not_valid_after(now + datetime.timedelta(days=args.days))
        .add_extension(x509.BasicConstraints(ca=True, path_length=0), critical=True)
        .add_extension(
            x509.KeyUsage(
                digital_signature=False,
                content_commitment=False,
                key_encipherment=False,
                data_encipherment=False,
                key_agreement=False,
                key_cert_sign=True,
                crl_sign=True,
                encipher_only=False,
                decipher_only=False,
            ),
            critical=True,
        )
        .add_extension(
            x509.SubjectKeyIdentifier.from_public_key(key.public_key()), critical=False
        )
        .sign(key, hashes.SHA256())
    )

    _write_secret(key_path, _serialize_key(key, passphrase), args.force)
    _write_public(cert_path, cert.public_bytes(serialization.Encoding.PEM), args.force)
    print(f"[OK] Root CA key:         {key_path} (keep offline, mode 0600)")
    print(f"[OK] Root CA certificate: {cert_path} (install on phones as trusted root)")
    return 0


def _build_san_names(san_ips: List[str], san_dns: List[str]) -> List[x509.GeneralName]:
    names: List[x509.GeneralName] = []
    for ip_text in san_ips:
        names.append(x509.IPAddress(ipaddress.ip_address(ip_text)))
    for dns_name in san_dns:
        if not DNS_NAME_PATTERN.match(dns_name) or len(dns_name) > 253:
            _fail(f"Invalid DNS SAN: '{dns_name}'.")
        names.append(x509.DNSName(dns_name))
    return names


def _issue_device_certificate(
    ca_key: ec.EllipticCurvePrivateKey,
    ca_cert: x509.Certificate,
    subject_public_key,
    common_name: str,
    san_names: List[x509.GeneralName],
    days: int,
) -> x509.Certificate:
    now = _now()
    return (
        x509.CertificateBuilder()
        .subject_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, common_name)]))
        .issuer_name(ca_cert.subject)
        .public_key(subject_public_key)
        .serial_number(x509.random_serial_number())
        .not_valid_before(now - datetime.timedelta(minutes=5))
        .not_valid_after(now + datetime.timedelta(days=days))
        .add_extension(x509.BasicConstraints(ca=False, path_length=None), critical=True)
        .add_extension(
            x509.KeyUsage(
                digital_signature=True,
                content_commitment=False,
                key_encipherment=False,
                data_encipherment=False,
                key_agreement=False,
                key_cert_sign=False,
                crl_sign=False,
                encipher_only=False,
                decipher_only=False,
            ),
            critical=True,
        )
        .add_extension(
            x509.ExtendedKeyUsage([ExtendedKeyUsageOID.SERVER_AUTH]), critical=False
        )
        .add_extension(x509.SubjectAlternativeName(san_names), critical=False)
        .add_extension(
            x509.SubjectKeyIdentifier.from_public_key(subject_public_key),
            critical=False,
        )
        .add_extension(
            x509.AuthorityKeyIdentifier.from_issuer_public_key(ca_key.public_key()),
            critical=False,
        )
        .sign(ca_key, hashes.SHA256())
    )


def cmd_sign_device(args: argparse.Namespace) -> int:
    if not args.san_ip and not args.san_dns:
        _fail("At least one --san-ip or --san-dns is required.")
    if args.days <= 0:
        _fail("--days must be positive.")
    san_names = _build_san_names(args.san_ip, args.san_dns)

    ca_dir = args.ca_dir.expanduser().resolve()
    ca_key, ca_cert = _load_ca(ca_dir, _passphrase(args.passphrase_env))
    out_dir = _prepare_out_dir(args.out_dir)
    key_path = out_dir / DEVICE_KEY_FILE
    cert_path = out_dir / DEVICE_CERT_FILE
    if key_path.exists() and not args.force:
        _fail(f"Device identity already exists in {out_dir} (use --force to replace).")

    device_key = ec.generate_private_key(ec.SECP256R1())
    common_name = args.common_name or (
        args.san_dns[0] if args.san_dns else args.san_ip[0]
    )
    cert = _issue_device_certificate(
        ca_key, ca_cert, device_key.public_key(), common_name, san_names, args.days
    )

    _write_secret(key_path, _serialize_key(device_key, None), args.force)
    _write_public(cert_path, cert.public_bytes(serialization.Encoding.PEM), args.force)
    print(f"[OK] Device key:         {key_path} (mode 0600)")
    print(f"[OK] Device certificate: {cert_path}")
    print(
        f"[OK] Verify with: openssl verify -CAfile {ca_dir / ROOT_CERT_FILE} {cert_path}"
    )
    return 0


def _load_verified_csr(csr_path: Path) -> x509.CertificateSigningRequest:
    if not csr_path.is_file():
        _fail(f"CSR file not found: {csr_path}")
    try:
        csr = x509.load_pem_x509_csr(csr_path.read_bytes())
    except ValueError as error:
        _fail(f"Cannot parse PKCS#10 CSR PEM in {csr_path}: {error}")
    if not csr.is_signature_valid:
        _fail(f"CSR signature verification failed for {csr_path}; refusing to sign.")
    public_key = csr.public_key()
    if not isinstance(public_key, ec.EllipticCurvePublicKey) or not isinstance(
        public_key.curve, ec.SECP256R1
    ):
        _fail(f"CSR public key in {csr_path} must be EC P-256 (ec-secp256r1).")
    return csr


def _append_traceability(log_path: Path, entry: dict) -> None:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    line = json.dumps(entry, sort_keys=True, separators=(",", ":"), ensure_ascii=True)
    with open(log_path, "a", encoding="utf-8") as handle:
        handle.write(line + "\n")
        handle.flush()
        os.fsync(handle.fileno())


def cmd_sign_csr(args: argparse.Namespace) -> int:
    if not DEVICE_ID_PATTERN.match(args.device_id):
        _fail("--device-id must be 1-64 characters from [A-Za-z0-9._:-].")
    if not args.san_ip and not args.san_dns:
        _fail("At least one --san-ip or --san-dns is required.")
    if args.days <= 0:
        _fail("--days must be positive.")
    san_names = _build_san_names(args.san_ip, args.san_dns)

    csr_path = args.csr.expanduser().resolve()
    csr = _load_verified_csr(csr_path)
    ca_dir = args.ca_dir.expanduser().resolve()
    ca_key, ca_cert = _load_ca(ca_dir, _passphrase(args.passphrase_env))

    out_path = args.out.expanduser().resolve()
    _prepare_out_dir(out_path.parent)
    common_name = args.common_name or (
        args.san_dns[0] if args.san_dns else args.san_ip[0]
    )
    cert = _issue_device_certificate(
        ca_key, ca_cert, csr.public_key(), common_name, san_names, args.days
    )

    _write_public(out_path, cert.public_bytes(serialization.Encoding.PEM), args.force)
    log_path = args.log.expanduser().resolve()
    _append_traceability(
        log_path,
        {
            "device_id": args.device_id,
            "timestamp_utc": _now().strftime("%Y-%m-%dT%H:%M:%SZ"),
            "tool": TOOL_NAME,
            "tool_version": TOOL_VERSION,
            "csr_sha256": hashlib.sha256(
                csr.public_bytes(serialization.Encoding.DER)
            ).hexdigest(),
            "cert_sha256": cert.fingerprint(hashes.SHA256()).hex(),
        },
    )
    print(f"[OK] Device certificate: {out_path}")
    print(f"[OK] Traceability entry appended to {log_path}")
    print(
        f"[OK] Verify with: openssl verify -CAfile {ca_dir / ROOT_CERT_FILE} {out_path}"
    )
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Private CA for device HTTPS server certificates."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    init_parser = subparsers.add_parser(
        "init-ca", help="Create an offline root CA outside the repository"
    )
    init_parser.add_argument(
        "--out-dir",
        required=True,
        type=Path,
        help="Directory for root_ca.key and root_ca.pem (outside the repo)",
    )
    init_parser.add_argument(
        "--common-name", default=DEFAULT_CA_COMMON_NAME, help="Root CA common name"
    )
    init_parser.add_argument(
        "--days", type=int, default=DEFAULT_CA_DAYS, help="Root CA validity in days"
    )
    init_parser.add_argument(
        "--passphrase-env",
        default="",
        help="Env var holding the passphrase that encrypts root_ca.key",
    )
    init_parser.add_argument(
        "--force", action="store_true", help="Replace an existing root CA"
    )
    init_parser.set_defaults(handler=cmd_init_ca)

    sign_parser = subparsers.add_parser(
        "sign-device",
        help="Sign a per-device HTTPS server certificate with the root CA",
    )
    sign_parser.add_argument(
        "--ca-dir",
        required=True,
        type=Path,
        help="Directory containing root_ca.key and root_ca.pem",
    )
    sign_parser.add_argument(
        "--out-dir",
        required=True,
        type=Path,
        help="Output directory for device_server.key and device_server.pem (outside the repo)",
    )
    sign_parser.add_argument(
        "--san-ip",
        action="append",
        default=[],
        help="IP SAN, e.g. 192.168.4.1 (repeatable)",
    )
    sign_parser.add_argument(
        "--san-dns",
        action="append",
        default=[],
        help="DNS SAN, e.g. the device mDNS name (repeatable)",
    )
    sign_parser.add_argument(
        "--common-name", default="", help="Subject CN (defaults to the first SAN)"
    )
    sign_parser.add_argument(
        "--days",
        type=int,
        default=DEFAULT_DEVICE_DAYS,
        help="Device certificate validity in days",
    )
    sign_parser.add_argument(
        "--passphrase-env",
        default="",
        help="Env var holding the passphrase that encrypts root_ca.key",
    )
    sign_parser.add_argument(
        "--force",
        action="store_true",
        help="Replace an existing device identity in --out-dir",
    )
    sign_parser.set_defaults(handler=cmd_sign_device)

    csr_parser = subparsers.add_parser(
        "sign-csr",
        help="Sign a device-generated CSR with the root CA (device keeps its key)",
    )
    csr_parser.add_argument(
        "--ca-dir",
        required=True,
        type=Path,
        help="Directory containing root_ca.key and root_ca.pem",
    )
    csr_parser.add_argument(
        "--csr",
        required=True,
        type=Path,
        help="PKCS#10 CSR PEM file produced by the device",
    )
    csr_parser.add_argument(
        "--out",
        required=True,
        type=Path,
        help="Output device certificate PEM path (outside the repo)",
    )
    csr_parser.add_argument(
        "--device-id",
        required=True,
        help="Device identifier recorded in the traceability log",
    )
    csr_parser.add_argument(
        "--log",
        required=True,
        type=Path,
        help="Append-only JSON Lines traceability log",
    )
    csr_parser.add_argument(
        "--san-ip",
        action="append",
        default=[],
        help="IP SAN (repeatable); CSR-requested SANs are ignored",
    )
    csr_parser.add_argument(
        "--san-dns",
        action="append",
        default=[],
        help="DNS SAN (repeatable); CSR-requested SANs are ignored",
    )
    csr_parser.add_argument(
        "--common-name", default="", help="Subject CN (defaults to the first SAN)"
    )
    csr_parser.add_argument(
        "--days", type=int, required=True, help="Device certificate validity in days"
    )
    csr_parser.add_argument(
        "--passphrase-env",
        default="",
        help="Env var holding the passphrase that encrypts root_ca.key",
    )
    csr_parser.add_argument(
        "--force", action="store_true", help="Replace an existing --out file"
    )
    csr_parser.set_defaults(handler=cmd_sign_csr)
    return parser


def main(argv: Optional[List[str]] = None) -> int:
    args = build_parser().parse_args(argv)
    return args.handler(args)


if __name__ == "__main__":
    sys.exit(main())
