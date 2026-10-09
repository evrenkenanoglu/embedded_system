"""
@file       security.py
@brief      Server-side X.509 certificate revocation and validity verification.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import datetime
import logging
from typing import Tuple
from cryptography import x509
from src.core.config import settings

logger = logging.getLogger("uvicorn.error")


def verify_certificate_status(cert_pem: str) -> Tuple[bool, str]:
    """
    Validates developer signing certificate expiration window and asserts
    it has not been revoked via dynamic CRL file or serial blocklist.
    """
    if not cert_pem or not cert_pem.strip():
        return False, "Certificate PEM payload is missing or empty"

    try:
        cert = x509.load_pem_x509_certificate(cert_pem.encode("utf-8"))
    except Exception as exc:
        return False, f"Malformed X.509 certificate: {exc}"

    now = datetime.datetime.now(datetime.timezone.utc)

    # 1. Expiration validity window check
    if now < cert.not_valid_before_utc:
        return (
            False,
            f"Certificate is not yet valid (notBefore: {cert.not_valid_before_utc})",
        )
    if now > cert.not_valid_after_utc:
        return False, f"Certificate has expired (notAfter: {cert.not_valid_after_utc})"

    serial_hex = format(cert.serial_number, "X").upper()

    # 2. Configured serial blocklist check
    if serial_hex in settings.REVOKED_SERIALS:
        return (
            False,
            f"Certificate serial 0x{serial_hex} is listed on revocation blocklist",
        )

    # 3. Dynamic CRL file evaluation if present on disk
    if settings.CRL_FILE.exists():
        try:
            crl_bytes = settings.CRL_FILE.read_bytes()
            crl = (
                x509.load_pem_x509_crl(crl_bytes)
                if b"-----BEGIN X509 CRL-----" in crl_bytes
                else x509.load_der_x509_crl(crl_bytes)
            )
            revoked_entry = crl.get_revoked_certificate_by_serial_number(
                cert.serial_number
            )
            if revoked_entry is not None:
                return (
                    False,
                    f"Certificate serial 0x{serial_hex} revoked in CRL on {revoked_entry.revocation_date_utc}",
                )
        except Exception as exc:
            return False, f"CRL verification error: {exc}"

    return True, "Valid"
