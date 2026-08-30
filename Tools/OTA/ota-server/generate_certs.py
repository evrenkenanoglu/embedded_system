import datetime
import ipaddress
import socket
from pathlib import Path
from cryptography import x509
from cryptography.x509.oid import NameOID
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from src.core.config import settings


def get_local_ip() -> str:
    """Helper to detect primary routing IP address."""
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # Does not send actual packets, used only to determine routing IP
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
    except Exception:
        ip = "127.0.0.1"
    finally:
        s.close()
    return ip


def write_pem_file(path: Path, data: bytes):
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "wb") as f:
        f.write(data)


def generate_certificates():
    cert_dir = settings.CERT_DIR
    local_ip = get_local_ip()

    print(f"Generating certificates in '{cert_dir}'. Local IP: {local_ip}")

    # 1. Generate CA Key and Self-Signed Certificate
    ca_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=settings.CA_KEY_SIZE
    )
    ca_subject = x509.Name([
        x509.NameAttribute(NameOID.COMMON_NAME, settings.CA_COMMON_NAME),
    ])
    ca_cert = (
        x509.CertificateBuilder()
        .subject_name(ca_subject)
        .issuer_name(ca_subject)
        .public_key(ca_key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(datetime.datetime.now(datetime.timezone.utc))
        .not_valid_after(
            datetime.datetime.now(datetime.timezone.utc) +
            datetime.timedelta(days=settings.CA_VALIDITY_DAYS)
        )
        .add_extension(
            x509.BasicConstraints(ca=True, path_length=None),
            critical=True
        )
        .sign(ca_key, hashes.SHA256())
    )

    # 2. Generate Server Key and Certificate Signed by CA
    server_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=settings.SERVER_KEY_SIZE
    )
    server_subject = x509.Name([
        x509.NameAttribute(NameOID.COMMON_NAME, settings.SERVER_COMMON_NAME),
    ])

    sans = []
    for dns in settings.STATIC_DNS_SANS:
        sans.append(x509.DNSName(dns))

    for ip_str in settings.STATIC_IP_SANS:
        sans.append(x509.IPAddress(ipaddress.ip_address(ip_str)))

    if local_ip != "127.0.0.1" and local_ip not in settings.STATIC_IP_SANS:
        try:
            sans.append(x509.IPAddress(ipaddress.ip_address(local_ip)))
        except ValueError:
            print(f"Warning: Local IP '{local_ip}' is invalid. Skipping.")

    server_cert = (
        x509.CertificateBuilder()
        .subject_name(server_subject)
        .issuer_name(ca_subject)
        .public_key(server_key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(datetime.datetime.now(datetime.timezone.utc))
        .not_valid_after(
            datetime.datetime.now(datetime.timezone.utc) +
            datetime.timedelta(days=settings.SERVER_VALIDITY_DAYS)
        )
        .add_extension(
            x509.SubjectAlternativeName(sans),
            critical=False,
        )
        .sign(ca_key, hashes.SHA256())
    )

    # 3. Write outputs to disk
    write_pem_file(
        settings.CA_KEY_FILE,
        ca_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            encryption_algorithm=serialization.NoEncryption(),
        )
    )
    write_pem_file(settings.CA_CERT_FILE, ca_cert.public_bytes(serialization.Encoding.PEM))

    write_pem_file(
        settings.SSL_KEY_FILE,
        server_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            encryption_algorithm=serialization.NoEncryption(),
        )
    )
    write_pem_file(settings.SSL_CERT_FILE, server_cert.public_bytes(serialization.Encoding.PEM))

    print(f"Certificates generated successfully in '{cert_dir}'.")


if __name__ == "__main__":
    generate_certificates()