#!/usr/bin/env python3
"""
@file       hsm_sign_digest.py
@brief      Generates SHA-256 digests and ECDSA SECP256R1 / RSA signatures for firmware binaries.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import argparse
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
    print("Error: The 'cryptography' library is required. Install it using: pip install cryptography", file=sys.stderr)
    sys.exit(1)


def compute_sha256(file_path: Path) -> bytes:
    """Computes raw SHA-256 digest of the specified file."""
    sha256 = hashlib.sha256()
    with open(file_path, "rb") as f:
        while chunk := f.read(65536):
            sha256.update(chunk)
    return sha256.digest()


def sign_digest_ec_secp256r1(digest: bytes, private_key_pem: bytes) -> bytes:
    """Signs a 32-byte digest using ECDSA SECP256R1 and returns raw IEEE P1363 (R || S) format (64 bytes)."""
    private_key = serialization.load_pem_private_key(private_key_pem, password=None)
    if not isinstance(private_key, ec.EllipticCurvePrivateKey):
        raise ValueError("Provided key is not an Elliptic Curve private key.")

    der_signature = private_key.sign(digest, ec.ECDSA(utils.Prehashed(hashes.SHA256())))
    r, s = utils.decode_dss_signature(der_signature)
    return r.to_bytes(32, byteorder="big") + s.to_bytes(32, byteorder="big")


def sign_digest_rsa_2048(digest: bytes, private_key_pem: bytes) -> bytes:
    """Signs a 32-byte digest using RSA PKCS#1 v1.5 with SHA-256."""
    private_key = serialization.load_pem_private_key(private_key_pem, password=None)
    if not isinstance(private_key, rsa.RSAPrivateKey):
        raise ValueError("Provided key is not an RSA private key.")

    return private_key.sign(
        digest,
        padding.PKCS1v15(),
        utils.Prehashed(hashes.SHA256())
    )


def extract_cert_pem(cert_path: Path) -> str:
    """Reads and validates an X.509 certificate in PEM format."""
    with open(cert_path, "rb") as f:
        cert_data = f.read()
    load_pem_x509_certificate(cert_data)
    return cert_data.decode("utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Sign firmware binary digests for ESP32 OTA framework.")
    parser.add_argument("--binary", "-b", type=Path, required=True, help="Path to input firmware binary (.bin)")
    parser.add_argument("--key", "-k", type=Path, required=True, help="Path to signing private key PEM file")
    parser.add_argument("--cert", "-c", type=Path, required=False, help="Path to developer signing certificate PEM file")
    parser.add_argument("--key-type", choices=["ec-secp256r1", "rsa-2048"], default="ec-secp256r1", help="Cryptographic key algorithm")
    parser.add_argument("--out-sig", type=Path, required=False, help="Path to write raw signature binary")
    parser.add_argument("--out-json", type=Path, required=False, help="Path to write manifest-compatible metadata JSON")
    args = parser.parse_args()

    if not args.binary.exists():
        print(f"Error: Binary not found: {args.binary}", file=sys.stderr)
        return 1

    if not args.key.exists():
        print(f"Error: Key file not found: {args.key}", file=sys.stderr)
        return 1

    file_size = args.binary.stat().st_size
    digest = compute_sha256(args.binary)
    digest_hex = digest.hex()

    with open(args.key, "rb") as f:
        key_bytes = f.read()

    try:
        if args.key_type == "ec-secp256r1":
            raw_sig = sign_digest_ec_secp256r1(digest, key_bytes)
        else:
            raw_sig = sign_digest_rsa_2048(digest, key_bytes)
    except Exception as e:
        print(f"Error during signature generation: {e}", file=sys.stderr)
        return 1

    sig_hex = raw_sig.hex()

    print("==================================================")
    print(" FIRMWARE CODE-SIGNING DIGEST MANIFEST")
    print("==================================================")
    print(f"Target Binary    : {args.binary}")
    print(f"File Size        : {file_size} bytes")
    print(f"Algorithm        : {args.key_type.upper()}")
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
        cert_pem = extract_cert_pem(args.cert) if args.cert and args.cert.exists() else ""
        manifest_entry: Dict[str, Any] = {
            "file_name": args.binary.name,
            "target_size": file_size,
            "target_hash": digest_hex,
            "target_signature": sig_hex,
            "signing_cert": cert_pem,
            "key_type": args.key_type
        }
        with open(args.out_json, "w", encoding="utf-8") as f:
            json.dump(manifest_entry, f, indent=4)
        print(f"[OK] JSON metadata written to: {args.out_json}")

    return 0


if __name__ == "__main__":
    sys.exit(main())