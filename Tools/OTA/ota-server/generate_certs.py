import datetime
import ipaddress
import socket
from pathlib import Path
from cryptography import x509
from cryptography.x509.oid import NameOID
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa

# ==============================================================================
# CONFIGURATION PARAMETERS (Modify as needed)
# ==============================================================================
OUTPUT_DIRECTORY = "certificates"

CA_COMMON_NAME = "MyLocalRootCA"
CA_KEY_SIZE = 4096
CA_VALIDITY_DAYS = 3650  # 10 years

SERVER_COMMON_NAME = "ota-server.local"
SERVER_KEY_SIZE = 2048
SERVER_VALIDITY_DAYS = 365  # 1 year

# Static Subject Alternative Names (SANs)
# Note: The active local IP address will also be auto-detected and appended.
STATIC_DNS_SANS = [
    "localhost",
    "ota-server.local"
]

STATIC_IP_SANS = [
    "127.0.0.1"
]
# ==============================================================================


def get_local_ip() -> str:
    """Helper to detect the primary local IP address of this machine."""
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
    cert_dir = Path(OUTPUT_DIRECTORY)
    local_ip = get_local_ip()
    
    print(f"Generating certificates. Auto-detected local IP: {local_ip}")

    # 1. Generate CA Private Key
    ca_key = rsa.generate_private_key(public_exponent=65537, key_size=CA_KEY_SIZE)
    
    # 2. Generate Self-Signed CA Certificate
    ca_subject = x509.Name([
        x509.NameAttribute(NameOID.COMMON_NAME, CA_COMMON_NAME),
    ])
    
    ca_cert = (
        x509.CertificateBuilder()
        .subject_name(ca_subject)
        .issuer_name(ca_subject)
        .public_key(ca_key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(datetime.datetime.now(datetime.timezone.utc))
        .not_valid_after(datetime.datetime.now(datetime.timezone.utc) + datetime.timedelta(days=CA_VALIDITY_DAYS))
        .add_extension(
            x509.BasicConstraints(ca=True, path_length=None), critical=True
        )
        .sign(ca_key, hashes.SHA256())
    )

    # 3. Generate Server Private Key
    server_key = rsa.generate_private_key(public_exponent=65537, key_size=SERVER_KEY_SIZE)

    # 4. Generate Server Certificate Signed by local CA
    server_subject = x509.Name([
        x509.NameAttribute(NameOID.COMMON_NAME, SERVER_COMMON_NAME),
    ])

    # Construct Subject Alternative Names (SANs) from configuration
    sans = []
    
    for dns in STATIC_DNS_SANS:
        sans.append(x509.DNSName(dns))
        
    for ip_str in STATIC_IP_SANS:
        sans.append(x509.IPAddress(ipaddress.ip_address(ip_str)))

    # Append the dynamically detected local IP if not already explicitly listed
    if local_ip != "127.0.0.1" and local_ip not in STATIC_IP_SANS:
        try:
            sans.append(x509.IPAddress(ipaddress.ip_address(local_ip)))
        except ValueError:
            print(f"Warning: Detected local IP '{local_ip}' is not a valid IP structure. Skipping.")

    server_cert = (
        x509.CertificateBuilder()
        .subject_name(server_subject)
        .issuer_name(ca_subject)
        .public_key(server_key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(datetime.datetime.now(datetime.timezone.utc))
        .not_valid_after(datetime.datetime.now(datetime.timezone.utc) + datetime.timedelta(days=SERVER_VALIDITY_DAYS))
        .add_extension(
            x509.SubjectAlternativeName(sans),
            critical=False,
        )
        .sign(ca_key, hashes.SHA256())
    )

    # Serialize and Save CA files
    write_pem_file(
        cert_dir / "ca.key",
        ca_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            encryption_algorithm=serialization.NoEncryption(),
        )
    )
    write_pem_file(cert_dir / "ca.crt", ca_cert.public_bytes(serialization.Encoding.PEM))

    # Serialize and Save Server files
    write_pem_file(
        cert_dir / "server.key",
        server_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            encryption_algorithm=serialization.NoEncryption(),
        )
    )
    write_pem_file(cert_dir / "server.crt", server_cert.public_bytes(serialization.Encoding.PEM))

    print(f"Successfully generated all certificates in: {OUTPUT_DIRECTORY}")


if __name__ == "__main__":
    generate_certificates()