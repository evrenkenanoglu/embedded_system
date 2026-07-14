import datetime
from pathlib import Path
from cryptography import x509
from cryptography.x509.oid import NameOID
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from src.core.config import settings

def init_signing_infrastructure():
    """Generates the delegated firmware-signing key pair and signs it with the Root CA."""
    ca_key_path = settings.CERT_DIR / "ca.key"
    ca_crt_path = settings.CA_CERT_FILE
    sign_key_path = settings.SIGNING_KEY_FILE
    sign_crt_path = settings.SIGNING_CRT_FILE

    settings.CERT_DIR.mkdir(parents=True, exist_ok=True)

    if not ca_key_path.exists() or not ca_crt_path.exists():
        raise RuntimeError("Root CA infrastructure (ca.key/ca.crt) is missing. Run generate_certs.py first.")

    # 1. Generate Signing Private Key (EC SECP256R1)
    if not sign_key_path.exists():
        signing_key = ec.generate_private_key(ec.SECP256R1())
        with open(sign_key_path, "wb") as f:
            f.write(
                signing_key.private_bytes(
                    encoding=serialization.Encoding.PEM,
                    format=serialization.PrivateFormat.PKCS8,
                    encryption_algorithm=serialization.NoEncryption()
                )
            )
    else:
        with open(sign_key_path, "rb") as f:
            signing_key = serialization.load_pem_private_key(f.read(), password=None)

    # 2. Sign Certificate with Root CA
    if not sign_crt_path.exists():
        with open(ca_key_path, "rb") as f:
            ca_key = serialization.load_pem_private_key(f.read(), password=None)
        with open(ca_crt_path, "rb") as f:
            ca_cert = x509.load_pem_x509_certificate(f.read())

        subject = x509.Name([
            x509.NameAttribute(NameOID.COMMON_NAME, settings.SIGNING_CERT_COMMON_NAME),
            x509.NameAttribute(NameOID.ORGANIZATION_NAME, settings.SIGNING_CERT_ORG),
        ])

        # Enforce Code Signing extended key usages
        signing_cert = (
            x509.CertificateBuilder()
            .subject_name(subject)
            .issuer_name(ca_cert.subject)
            .public_key(signing_key.public_key())
            .serial_number(x509.random_serial_number())
            .not_valid_before(datetime.datetime.now(datetime.timezone.utc))
            .not_valid_after(
                datetime.datetime.now(datetime.timezone.utc) + 
                datetime.timedelta(days=settings.SIGNING_VALIDITY_DAYS)
            )
            .add_extension(
                x509.BasicConstraints(ca=False, path_length=None), critical=True
            )
            .add_extension(
                x509.ExtendedKeyUsage([x509.oid.ExtendedKeyUsageOID.CODE_SIGNING]), critical=True
            )
            .sign(ca_key, hashes.SHA256())
        )

        with open(sign_crt_path, "wb") as f:
            f.write(signing_cert.public_bytes(serialization.Encoding.PEM))

def get_signing_certificate_pem() -> str:
    """Returns the dynamic signing certificate PEM string."""
    init_signing_infrastructure()
    with open(settings.SIGNING_CRT_FILE, "r") as f:
        return f.read()

def sign_file(file_path: Path) -> str:
    """Signs target binary using SECP256R1 SHA-256."""
    init_signing_infrastructure()
    with open(settings.SIGNING_KEY_FILE, "rb") as f:
        signing_key = serialization.load_pem_private_key(f.read(), password=None)
    with open(file_path, "rb") as f:
        data = f.read()
    signature = signing_key.sign(data, ec.ECDSA(hashes.SHA256()))
    return signature.hex()