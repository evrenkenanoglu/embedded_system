"""
@file       test_private_ca.py
@brief      Unit tests for private_ca.py sign-csr: signature check, root verification and output placement.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import base64
import json
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.x509 import NameOID
from cryptography.x509.oid import ExtendedKeyUsageOID

import private_ca

TOOL_DIR = Path(__file__).resolve().parent
PASS_ENV = "PRIVATE_CA_TEST_PASSPHRASE"
DEVICE_ID = "DEV-TEST-001"
SAN_IP = "192.168.4.1"


def _make_csr(key: ec.EllipticCurvePrivateKey, tamper: bool = False) -> bytes:
    csr = (
        x509.CertificateSigningRequestBuilder()
        .subject_name(
            x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, "device-requested")])
        )
        .sign(key, hashes.SHA256())
    )
    if not tamper:
        return csr.public_bytes(serialization.Encoding.PEM)
    der = bytearray(csr.public_bytes(serialization.Encoding.DER))
    der[
        -1
    ] ^= 0x01  # flips the low bit of the ECDSA s value, so the signature no longer verifies
    body = base64.encodebytes(bytes(der))
    return (
        b"-----BEGIN CERTIFICATE REQUEST-----\n"
        + body
        + b"-----END CERTIFICATE REQUEST-----\n"
    )


class PrivateCaSignCsrTest(unittest.TestCase):
    def setUp(self) -> None:
        # Outside the repository on purpose: the tool refuses in-repo output.
        self._outside = tempfile.TemporaryDirectory(
            dir=Path.home(), prefix="private_ca_test_"
        )
        self.workdir = Path(self._outside.name)
        self.ca_dir = self.workdir / "ca"
        self.log_path = self.workdir / "signings.jsonl"
        self.env = mock.patch.dict(os.environ, {PASS_ENV: "unit-test-passphrase"})
        self.env.start()
        private_ca.main(
            ["init-ca", "--out-dir", str(self.ca_dir), "--passphrase-env", PASS_ENV]
        )

    def tearDown(self) -> None:
        self.env.stop()
        self._outside.cleanup()

    def _sign(self, csr_pem: bytes, out: Path) -> int:
        csr_path = self.workdir / f"{out.stem}.csr"
        csr_path.write_bytes(csr_pem)
        return private_ca.main(
            [
                "sign-csr",
                "--ca-dir",
                str(self.ca_dir),
                "--csr",
                str(csr_path),
                "--out",
                str(out),
                "--device-id",
                DEVICE_ID,
                "--log",
                str(self.log_path),
                "--san-ip",
                SAN_IP,
                "--days",
                "825",
                "--passphrase-env",
                PASS_ENV,
            ]
        )

    def test_valid_csr_signs_and_verifies_against_root(self) -> None:
        device_key = ec.generate_private_key(ec.SECP256R1())
        out = self.workdir / "device_cert.pem"

        self.assertEqual(0, self._sign(_make_csr(device_key), out))

        cert = x509.load_pem_x509_certificate(out.read_bytes())
        self.assertEqual(
            device_key.public_key().public_numbers(), cert.public_key().public_numbers()
        )
        san = cert.extensions.get_extension_for_class(x509.SubjectAlternativeName).value
        self.assertEqual(
            [SAN_IP], [str(ip) for ip in san.get_values_for_type(x509.IPAddress)]
        )
        self.assertIn(
            ExtendedKeyUsageOID.SERVER_AUTH,
            cert.extensions.get_extension_for_class(x509.ExtendedKeyUsage).value,
        )
        self.assertFalse(
            cert.extensions.get_extension_for_class(x509.BasicConstraints).value.ca
        )

        openssl = shutil.which("openssl")
        if openssl:
            result = subprocess.run(
                [
                    openssl,
                    "verify",
                    "-CAfile",
                    str(self.ca_dir / private_ca.ROOT_CERT_FILE),
                    str(out),
                ],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(0, result.returncode, result.stdout + result.stderr)

        records = [
            json.loads(line)
            for line in self.log_path.read_text(encoding="utf-8").splitlines()
        ]
        self.assertEqual(1, len(records))
        self.assertEqual(DEVICE_ID, records[0]["device_id"])
        self.assertEqual(
            cert.fingerprint(hashes.SHA256()).hex(), records[0]["cert_sha256"]
        )
        self.assertNotIn("BEGIN", self.log_path.read_text(encoding="utf-8"))

    def test_csr_with_bad_signature_is_rejected(self) -> None:
        device_key = ec.generate_private_key(ec.SECP256R1())
        out = self.workdir / "rejected_cert.pem"

        with self.assertRaisesRegex(SystemExit, "signature verification failed"):
            self._sign(_make_csr(device_key, tamper=True), out)

        self.assertFalse(out.exists())
        self.assertFalse(self.log_path.exists())

    def test_output_inside_repo_is_refused(self) -> None:
        device_key = ec.generate_private_key(ec.SECP256R1())
        with tempfile.TemporaryDirectory(dir=TOOL_DIR) as in_repo:
            out = Path(in_repo) / "device_cert.pem"
            with self.assertRaisesRegex(SystemExit, "git worktree"):
                self._sign(_make_csr(device_key), out)
            self.assertFalse(out.exists())


if __name__ == "__main__":
    unittest.main()
