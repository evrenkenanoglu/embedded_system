#!/usr/bin/env python3
"""
@file       generate_pki.py
@brief      Generic data-driven PKI and hardware silicon key generator.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import argparse
import datetime
import ipaddress
import secrets
import subprocess
import sys
from pathlib import Path
from typing import Dict, Any, Tuple, List, Union, Optional

SCRIPT_DIR = Path(__file__).resolve().parent

try:
    import yaml
except ImportError:
    print("[ERROR] PyYAML is required. Run: pip install pyyaml", file=sys.stderr)
    sys.exit(1)

try:
    from cryptography import x509
    from cryptography.hazmat.primitives import hashes, serialization
    from cryptography.hazmat.primitives.asymmetric import ec, rsa
    from cryptography.x509.oid import ExtendedKeyUsageOID, NameOID
except ImportError:
    print("[ERROR] Cryptography library is required. Run: pip install cryptography", file=sys.stderr)
    sys.exit(1)


def auto_detect_roots() -> Tuple[Path, Path]:
    """Auto-detects (project_root, embedded_system_root) traversing upward from SCRIPT_DIR."""
    curr = SCRIPT_DIR
    embedded_system_root = None
    project_root = None

    while curr != curr.parent:
        if curr.name == "embedded_system" or ((curr / "Source").exists() and (curr / "CMakeLists.txt").exists()):
            embedded_system_root = curr
            project_root = curr.parent
            break
        curr = curr.parent

    if not embedded_system_root:
        if len(SCRIPT_DIR.parents) >= 2:
            embedded_system_root = SCRIPT_DIR.parents[1]
            project_root = SCRIPT_DIR.parents[2]
        else:
            embedded_system_root = Path.cwd()
            project_root = Path.cwd()

    return project_root.resolve(), embedded_system_root.resolve()


def expand_variables(data: Any, env_map: Dict[str, str]) -> Any:
    """Recursively replaces {VARIABLE} placeholders in config data."""
    if isinstance(data, str):
        for key, val in env_map.items():
            data = data.replace(f"{{{key}}}", str(val))
        return data
    elif isinstance(data, dict):
        return {k: expand_variables(v, env_map) for k, v in data.items()}
    elif isinstance(data, list):
        return [expand_variables(item, env_map) for item in data]
    return data


def load_config(config_path: Path, cli_project_root: Optional[Path] = None) -> Tuple[Dict[str, Any], Path, Tuple[Path, Path]]:
    """Loads YAML configuration, auto-anchoring roots and expanding path placeholders."""
    if not config_path.exists():
        candidate = (Path.cwd() / config_path).resolve()
        if candidate.exists():
            config_path = candidate
        else:
            raise FileNotFoundError(f"Configuration file missing: {config_path}")

    with open(config_path, "r", encoding="utf-8") as f:
        cfg = yaml.safe_load(f)

    auto_proj_root, auto_embed_root = auto_detect_roots()
    project_root = cli_project_root.resolve() if cli_project_root else auto_proj_root
    embedded_system_root = auto_embed_root

    env_map: Dict[str, str] = {
        "project_root_dir": str(project_root),
        "embedded_system_dir": str(embedded_system_root)
    }

    for _ in range(4):
        cfg = expand_variables(cfg, env_map)
        paths_cfg = cfg.get("paths", {})
        for k, v in paths_cfg.items():
            if isinstance(v, str) and not ("{" in v and "}" in v):
                env_map[k] = str(v)

    return cfg, config_path.parent.resolve(), (project_root, embedded_system_root)


def generate_private_key(key_type: str) -> Union[ec.EllipticCurvePrivateKey, rsa.RSAPrivateKey]:
    """Generates an asymmetric private key according to configured algorithm."""
    key_type = key_type.lower()
    if key_type == "ec-secp256r1":
        return ec.generate_private_key(ec.SECP256R1())
    elif key_type == "ec-secp384r1":
        return ec.generate_private_key(ec.SECP384R1())
    elif key_type == "rsa-2048":
        return rsa.generate_private_key(public_exponent=65537, key_size=2048)
    elif key_type == "rsa-4096":
        return rsa.generate_private_key(public_exponent=65537, key_size=4096)
    else:
        raise ValueError(f"Unsupported key_type: {key_type}")


def build_x509_name(subject_cfg: Dict[str, str]) -> x509.Name:
    """Dynamically converts configuration dictionary into x509.Name attributes."""
    attributes = []
    if "common_name" in subject_cfg:
        attributes.append(x509.NameAttribute(NameOID.COMMON_NAME, subject_cfg["common_name"]))
    if "organization" in subject_cfg:
        attributes.append(x509.NameAttribute(NameOID.ORGANIZATION_NAME, subject_cfg["organization"]))
    if "country" in subject_cfg:
        attributes.append(x509.NameAttribute(NameOID.COUNTRY_NAME, subject_cfg["country"]))
    if "state" in subject_cfg:
        attributes.append(x509.NameAttribute(NameOID.STATE_OR_PROVINCE_NAME, subject_cfg["state"]))
    if "locality" in subject_cfg:
        attributes.append(x509.NameAttribute(NameOID.LOCALITY_NAME, subject_cfg["locality"]))

    return x509.Name(attributes)


def save_pem_key(key: Any, file_path: Path) -> None:
    """Saves private key in PEM format."""
    file_path.parent.mkdir(parents=True, exist_ok=True)
    with open(file_path, "wb") as f:
        f.write(
            key.private_bytes(
                encoding=serialization.Encoding.PEM,
                format=serialization.PrivateFormat.TraditionalOpenSSL,
                encryption_algorithm=serialization.NoEncryption(),
            )
        )


def save_pem_cert(cert: x509.Certificate, file_path: Path) -> None:
    """Saves certificate in PEM format."""
    file_path.parent.mkdir(parents=True, exist_ok=True)
    with open(file_path, "wb") as f:
        f.write(cert.public_bytes(serialization.Encoding.PEM))


def generate_root_ca(ca_cfg: Dict[str, Any]) -> Tuple[Any, x509.Certificate]:
    """Generates a self-signed Root Certificate Authority."""
    key = generate_private_key(ca_cfg.get("key_type", "ec-secp256r1"))
    subject = issuer = build_x509_name(ca_cfg["subject"])

    now = datetime.datetime.now(datetime.timezone.utc)
    validity = ca_cfg.get("validity_days", 3650)

    cert = (
        x509.CertificateBuilder()
        .subject_name(subject)
        .issuer_name(issuer)
        .public_key(key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(now)
        .not_valid_after(now + datetime.timedelta(days=validity))
        .add_extension(x509.BasicConstraints(ca=True, path_length=None), critical=True)
        .add_extension(
            x509.KeyUsage(
                digital_signature=True,
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
        .sign(key, hashes.SHA256())
    )

    save_pem_key(key, Path(ca_cfg["key_file"]).resolve())
    save_pem_cert(cert, Path(ca_cfg["cert_file"]).resolve())
    return key, cert


def generate_code_signing_cert(signing_cfg: Dict[str, Any], ca_key: Any, ca_cert: x509.Certificate) -> Tuple[Any, x509.Certificate]:
    """Generates a developer code-signing certificate signed by the Root CA."""
    key = generate_private_key(signing_cfg.get("key_type", "ec-secp256r1"))
    subject = build_x509_name(signing_cfg["subject"])

    now = datetime.datetime.now(datetime.timezone.utc)
    validity = signing_cfg.get("validity_days", 365)

    cert = (
        x509.CertificateBuilder()
        .subject_name(subject)
        .issuer_name(ca_cert.subject)
        .public_key(key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(now)
        .not_valid_after(now + datetime.timedelta(days=validity))
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
            x509.ExtendedKeyUsage([ExtendedKeyUsageOID.CODE_SIGNING]),
            critical=False,
        )
        .sign(ca_key, hashes.SHA256())
    )

    save_pem_key(key, Path(signing_cfg["key_file"]).resolve())
    save_pem_cert(cert, Path(signing_cfg["cert_file"]).resolve())
    return key, cert


def generate_server_tls_cert(server_cfg: Dict[str, Any], ca_key: Any, ca_cert: x509.Certificate) -> Tuple[Any, x509.Certificate]:
    """Generates an HTTPS TLS server certificate signed by the Root CA."""
    key = generate_private_key(server_cfg.get("key_type", "ec-secp256r1"))
    subject = build_x509_name(server_cfg["subject"])

    san_list: List[x509.GeneralName] = []
    san_cfg = server_cfg.get("subject_alternative_names", {})

    for dns_entry in san_cfg.get("dns", []):
        san_list.append(x509.DNSName(dns_entry))
    for ip_entry in san_cfg.get("ip", []):
        san_list.append(x509.IPAddress(ipaddress.ip_address(ip_entry)))

    now = datetime.datetime.now(datetime.timezone.utc)
    validity = server_cfg.get("validity_days", 365)

    builder = (
        x509.CertificateBuilder()
        .subject_name(subject)
        .issuer_name(ca_cert.subject)
        .public_key(key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(now)
        .not_valid_after(now + datetime.timedelta(days=validity))
        .add_extension(x509.BasicConstraints(ca=False, path_length=None), critical=True)
        .add_extension(
            x509.ExtendedKeyUsage([ExtendedKeyUsageOID.SERVER_AUTH]),
            critical=False,
        )
    )

    if san_list:
        builder = builder.add_extension(x509.SubjectAlternativeName(san_list), critical=False)

    cert = builder.sign(ca_key, hashes.SHA256())

    save_pem_key(key, Path(server_cfg["key_file"]).resolve())
    save_pem_cert(cert, Path(server_cfg["cert_file"]).resolve())
    return key, cert


def generate_silicon_hardware_keys(silicon_cfg: Dict[str, Any]) -> None:
    """Generates AES flash encryption key and Secure Boot V2 ECDSA/RSA key and public digest."""
    # 1. Flash Encryption AES Key
    flash_cfg = silicon_cfg.get("flash_encryption", {})
    key_size = flash_cfg.get("key_size_bytes", 32)
    flash_file = Path(flash_cfg["key_file"]).resolve()
    flash_file.parent.mkdir(parents=True, exist_ok=True)

    with open(flash_file, "wb") as f:
        f.write(secrets.token_bytes(key_size))
    print(f"[OK] Flash Encryption Key generated: {flash_file.name}")

    # 2. Secure Boot V2 Signing Key & Digest
    sb_cfg = silicon_cfg.get("secure_boot_v2", {})
    scheme = sb_cfg.get("scheme", "ecdsa256")
    sb_pem = Path(sb_cfg["key_file"]).resolve()
    sb_digest = Path(sb_cfg["digest_file"]).resolve()
    sb_pem.parent.mkdir(parents=True, exist_ok=True)
    sb_digest.parent.mkdir(parents=True, exist_ok=True)

    cmd_gen = [
        sys.executable, "-m", "espsecure",
        "generate_signing_key",
        "--version", "2",
        "--scheme", scheme,
        str(sb_pem.resolve())
    ]
    res_gen = subprocess.run(cmd_gen, capture_output=True, text=True, check=False)
    if res_gen.returncode != 0:
        raise RuntimeError(f"Failed to generate Secure Boot V2 key:\n{res_gen.stderr}")

    cmd_dig = [
        sys.executable, "-m", "espsecure",
        "digest_sbv2_public_key",
        "--keyfile", str(sb_pem.resolve()),
        "--output", str(sb_digest.resolve())
    ]
    res_dig = subprocess.run(cmd_dig, capture_output=True, text=True, check=False)
    if res_dig.returncode != 0:
        raise RuntimeError(f"Failed to extract Secure Boot V2 digest:\n{res_dig.stderr}")

    print(f"[OK] Secure Boot V2 key and digest generated: {sb_digest.name}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generic Data-Driven PKI & Silicon Key Generator.")
    parser.add_argument("--config", "-c", type=Path, default=SCRIPT_DIR / "config.yaml", help="Path to config.yaml")
    parser.add_argument("--project-root", "-r", type=Path, default=None, help="Explicit project root directory override")
    args = parser.parse_args()

    try:
        config, config_dir, roots = load_config(args.config, args.project_root)

        print("==================================================")
        print(" GENERIC DATA-DRIVEN PKI & SILICON KEY GENERATOR")
        print("==================================================")
        print(f"Configuration File : {args.config.resolve()}")
        print(f"Project Root       : {roots[0]}")
        print(f"Embedded Root      : {roots[1]}")
        print("==================================================")

        # 1. Root CA Generation
        root_cfg = config.get("root_ca", {})
        if root_cfg.get("enabled", True):
            ca_key, ca_cert = generate_root_ca(root_cfg)
            print(f"[OK] Root CA Authority generated: {Path(root_cfg['cert_file']).name}")

            # 2. Code-Signing Certificate Generation
            signing_cfg = config.get("code_signing", {})
            if signing_cfg.get("enabled", True):
                generate_code_signing_cert(signing_cfg, ca_key, ca_cert)
                print(f"[OK] Developer Signing Certificate generated: {Path(signing_cfg['cert_file']).name}")

            # 3. Server HTTPS TLS Generation
            server_cfg = config.get("server_tls", {})
            if server_cfg.get("enabled", True):
                generate_server_tls_cert(server_cfg, ca_key, ca_cert)
                print(f"[OK] Server TLS Certificate generated: {Path(server_cfg['cert_file']).name}")

        # 4. Silicon Hardware Keys Generation
        silicon_cfg = config.get("silicon_keys", {})
        if silicon_cfg.get("enabled", True):
            generate_silicon_hardware_keys(silicon_cfg)

        print("\n[SUCCESS] All PKI certificates and silicon keys generated successfully.")
        return 0

    except Exception as e:
        print(f"\n[FATAL ERROR] {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())