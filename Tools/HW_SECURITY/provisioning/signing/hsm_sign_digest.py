#!/usr/bin/env python3
"""
@file       hsm_sign_digest.py
@brief      Generates SHA-256 digests and ECDSA SECP256R1 / RSA signatures for firmware binaries.
            Supports detached offline HSM workflows, remote Cloud KMS (AWS KMS, HashiCorp Vault),
            and local release signing.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import argparse
import base64
import hashlib
import json
import os
import sys
from pathlib import Path
from typing import Dict, Any, Optional

try:
    from cryptography.hazmat.primitives import hashes, serialization
    from cryptography.hazmat.primitives.asymmetric import ec, rsa, utils, padding
    from cryptography.x509 import load_pem_x509_certificate
except ImportError:
    print(
        "Error: The 'cryptography' library is required. Install it using: pip install cryptography",
        file=sys.stderr,
    )
    sys.exit(1)


def compute_sha256(file_path: Path) -> bytes:
    """Computes raw 32-byte SHA-256 digest of the specified file."""
    sha256 = hashlib.sha256()
    with open(file_path, "rb") as f:
        while chunk := f.read(65536):
            sha256.update(chunk)
    return sha256.digest()


def der_to_ieee_p1363(der_sig: bytes) -> bytes:
    """Decodes ASN.1 DER ECDSA signature and returns raw IEEE P1363 (R || S) format (64 bytes)."""
    r, s = utils.decode_dss_signature(der_sig)
    return r.to_bytes(32, byteorder="big") + s.to_bytes(32, byteorder="big")


def sign_digest_ec_secp256r1_local(digest: bytes, private_key_pem: bytes) -> bytes:
    """Signs a 32-byte digest using a local ECDSA SECP256R1 private key."""
    private_key = serialization.load_pem_private_key(private_key_pem, password=None)
    if not isinstance(private_key, ec.EllipticCurvePrivateKey):
        raise ValueError("Provided key is not an Elliptic Curve private key.")

    der_signature = private_key.sign(digest, ec.ECDSA(utils.Prehashed(hashes.SHA256())))
    return der_to_ieee_p1363(der_signature)


def sign_digest_rsa_2048_local(digest: bytes, private_key_pem: bytes) -> bytes:
    """Signs a 32-byte digest using a local RSA private key."""
    private_key = serialization.load_pem_private_key(private_key_pem, password=None)
    if not isinstance(private_key, rsa.RSAPrivateKey):
        raise ValueError("Provided key is not an RSA private key.")

    return private_key.sign(
        digest, padding.PKCS1v15(), utils.Prehashed(hashes.SHA256())
    )


# Backward-compatible aliases for main.py and external consumers
sign_digest_ec_secp256r1 = sign_digest_ec_secp256r1_local
sign_digest_rsa_2048 = sign_digest_rsa_2048_local


def sign_digest_aws_kms(digest: bytes, key_id: str) -> bytes:
    """Remotely signs 32-byte digest via AWS KMS and converts DER response to IEEE P1363."""
    try:
        import boto3
    except ImportError:
        print(
            "[ERROR] 'boto3' is required for AWS KMS signing. Run: pip install boto3",
            file=sys.stderr,
        )
        sys.exit(1)

    kms_client = boto3.client("kms")
    response = kms_client.sign(
        KeyId=key_id,
        Message=digest,
        MessageType="DIGEST",
        SigningAlgorithm="ECDSA_SHA_256",
    )

    der_sig = response["Signature"]
    return der_to_ieee_p1363(der_sig)


def sign_digest_vault(digest: bytes, vault_url: str, token: str, key_name: str) -> bytes:
    """Remotely signs 32-byte digest via HashiCorp Vault Transit Secrets Engine."""
    try:
        import requests
    except ImportError:
        print(
            "[ERROR] 'requests' is required for Vault signing. Run: pip install requests",
            file=sys.stderr,
        )
        sys.exit(1)

    endpoint = f"{vault_url.rstrip('/')}/v1/transit/sign/{key_name}/sha2-256"
    headers = {"X-Vault-Token": token}
    payload = {"input": base64.b64encode(digest).decode("utf-8")}

    res = requests.post(endpoint, json=payload, headers=headers, timeout=10.0)
    if res.status_code != 200:
        raise RuntimeError(f"Vault signing failed: HTTP {res.status_code} - {res.text}")

    vault_signature_str = res.json()["data"]["signature"]
    raw_b64 = vault_signature_str.split(":")[-1]
    der_sig = base64.b64decode(raw_b64)
    return der_to_ieee_p1363(der_sig)


def extract_cert_pem(cert_path: Path) -> str:
    """Reads and validates an X.509 certificate in PEM format."""
    with open(cert_path, "rb") as f:
        cert_data = f.read()
    load_pem_x509_certificate(cert_data)
    return cert_data.decode("utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Two-stage HSM, Cloud KMS, and local code-signing orchestrator for ESP32 OTA framework."
    )
    parser.add_argument(
        "--stage",
        choices=["all", "digest", "assemble"],
        default="all",
        help="Signing pipeline stage: 'digest' (export hash for HSM), 'assemble' (inject detached signature), 'all' (sign now)",
    )
    parser.add_argument(
        "--binary",
        "-b",
        type=Path,
        required=True,
        help="Path to input firmware binary (.bin)",
    )
    parser.add_argument(
        "--key",
        "-k",
        type=Path,
        required=False,
        help="Path to local signing private key PEM file (required for local 'all' signing)",
    )
    parser.add_argument(
        "--cert",
        "-c",
        type=Path,
        required=False,
        help="Path to developer signing certificate PEM file",
    )
    parser.add_argument(
        "--key-type",
        choices=["ec-secp256r1", "rsa-2048"],
        default="ec-secp256r1",
        help="Cryptographic key algorithm",
    )

    # Cloud KMS / Vault Integration
    parser.add_argument(
        "--kms-provider",
        choices=["none", "aws-kms", "vault"],
        default="none",
        help="Remote hardware signing provider",
    )
    parser.add_argument(
        "--kms-key-id",
        type=str,
        default="",
        help="AWS KMS Key ID / ARN / Alias or Vault Transit Key Name",
    )
    parser.add_argument(
        "--vault-url",
        type=str,
        default=os.environ.get("VAULT_ADDR", "http://127.0.0.1:8200"),
        help="HashiCorp Vault URL",
    )
    parser.add_argument(
        "--vault-token",
        type=str,
        default=os.environ.get("VAULT_TOKEN", ""),
        help="HashiCorp Vault Access Token",
    )

    # Output paths
    parser.add_argument(
        "--out-digest",
        type=Path,
        required=False,
        help="Path to write raw 32-byte SHA-256 digest binary (for 'digest' stage)",
    )
    parser.add_argument(
        "--sig-in",
        type=Path,
        required=False,
        help="Path to detached raw signature binary (for 'assemble' stage)",
    )
    parser.add_argument(
        "--out-sig",
        type=Path,
        required=False,
        help="Path to write raw signature binary",
    )
    parser.add_argument(
        "--out-json",
        type=Path,
        required=False,
        help="Path to write manifest-compatible metadata JSON",
    )
    args = parser.parse_args()

    if not args.binary.exists():
        print(f"[ERROR] Target binary not found: {args.binary}", file=sys.stderr)
        return 1

    file_size = args.binary.stat().st_size
    digest = compute_sha256(args.binary)
    digest_hex = digest.hex()

    # --------------------------------------------------------------------------
    # STAGE 1: Export SHA-256 Digest for Air-Gapped Offline Signing
    # --------------------------------------------------------------------------
    if args.stage == "digest":
        target_out_digest = args.out_digest or args.binary.with_suffix(".digest.bin")
        target_out_digest.parent.mkdir(parents=True, exist_ok=True)
        with open(target_out_digest, "wb") as f:
            f.write(digest)

        print("==================================================")
        print(" FIRMWARE DIGEST EXPORT (HSM / KMS PREPARATION)")
        print("==================================================")
        print(f"Target Binary  : {args.binary}")
        print(f"Binary Size    : {file_size} bytes")
        print(f"SHA-256 Digest : {digest_hex}")
        print(f"Digest Binary  : {target_out_digest}")
        print("==================================================")
        print(
            f"[OK] Digest binary exported. Forward '{target_out_digest}' to HSM/KMS for signing."
        )
        return 0

    # --------------------------------------------------------------------------
    # STAGE 2: Ingest External Detached Signature
    # --------------------------------------------------------------------------
    raw_sig: bytes = b""
    if args.stage == "assemble":
        if not args.sig_in or not args.sig_in.exists():
            print(
                f"[ERROR] Detached signature file (--sig-in) required for 'assemble' stage.",
                file=sys.stderr,
            )
            return 1
        with open(args.sig_in, "rb") as f:
            raw_sig = f.read()

        if args.key_type == "ec-secp256r1" and len(raw_sig) != 64:
            print(
                f"[ERROR] EC SECP256R1 signature must be exactly 64 bytes IEEE P1363 (R || S). Got: {len(raw_sig)} bytes.",
                file=sys.stderr,
            )
            return 1

    # --------------------------------------------------------------------------
    # STAGE 3: Execute Signing (Remote KMS or Local Key)
    # --------------------------------------------------------------------------
    elif args.stage == "all":
        try:
            if args.kms_provider == "aws-kms":
                if not args.kms_key_id:
                    print(
                        "[ERROR] --kms-key-id required when using --kms-provider aws-kms",
                        file=sys.stderr,
                    )
                    return 1
                print(f"[*] Dispatching digest to AWS KMS key: {args.kms_key_id}")
                raw_sig = sign_digest_aws_kms(digest, args.kms_key_id)

            elif args.kms_provider == "vault":
                if not args.kms_key_id or not args.vault_token:
                    print(
                        "[ERROR] --kms-key-id and VAULT_TOKEN required when using --kms-provider vault",
                        file=sys.stderr,
                    )
                    return 1
                print(f"[*] Dispatching digest to HashiCorp Vault key: {args.kms_key_id}")
                raw_sig = sign_digest_vault(
                    digest, args.vault_url, args.vault_token, args.kms_key_id
                )

            else:
                # Local private key signing fallback
                if not args.key or not args.key.exists():
                    print(
                        "[ERROR] Local signing private key file (--key) required.",
                        file=sys.stderr,
                    )
                    return 1

                with open(args.key, "rb") as f:
                    key_bytes = f.read()

                if args.key_type == "ec-secp256r1":
                    raw_sig = sign_digest_ec_secp256r1_local(digest, key_bytes)
                else:
                    raw_sig = sign_digest_rsa_2048_local(digest, key_bytes)

        except Exception as e:
            print(f"[ERROR] Signature generation failed: {e}", file=sys.stderr)
            return 1

    sig_hex = raw_sig.hex()

    print("==================================================")
    print(" FIRMWARE CODE-SIGNING RELEASE METADATA")
    print("==================================================")
    print(f"Target Binary    : {args.binary}")
    print(f"File Size        : {file_size} bytes")
    print(f"Algorithm        : {args.key_type.upper()}")
    print(f"Provider         : {args.kms_provider.upper() if args.stage == 'all' else 'DETACHED'}")
    print(f"SHA-256 Digest   : {digest_hex}")
    print(f"Target Signature : {sig_hex}")
    print("==================================================")

    if args.out_sig:
        args.out_sig.parent.mkdir(parents=True, exist_ok=True)
        with open(args.out_sig, "wb") as f:
            f.write(raw_sig)
        print(f"[OK] Raw signature written to: {args.out_sig}")

    if args.out_json:
        args.out_json.parent.mkdir(parents=True, exist_ok=True)
        cert_pem = (
            extract_cert_pem(args.cert) if args.cert and args.cert.exists() else ""
        )
        manifest_entry: Dict[str, Any] = {
            "file_name": args.binary.name,
            "target_size": file_size,
            "target_hash": digest_hex,
            "target_signature": sig_hex,
            "signing_cert": cert_pem,
            "key_type": args.key_type,
        }
        with open(args.out_json, "w", encoding="utf-8") as f:
            json.dump(manifest_entry, f, indent=4)
        print(f"[OK] JSON metadata written to: {args.out_json}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
