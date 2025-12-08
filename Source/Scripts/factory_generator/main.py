import os
import datetime
import subprocess
import sys
from cryptography import x509
from cryptography.x509.oid import NameOID
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives import serialization

# --- CONFIGURATION ---
# These MUST match the NVS namespace and keys in your C++ code
NVS_NAMESPACE = "cpx_storage" 
KEY_NVS_NAME = "https_key"
CERT_NVS_NAME = "https_cert"
# The size of your NVS partition (check your partitions.csv)
NVS_PARTITION_SIZE = "0x4000" 

def generate_credentials(device_id):
    print(f"Generating keys for Device: {device_id}...")

    # 1. Generate Private Key
    private_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=2048,
    )

    # 2. Generate Self-Signed Certificate
    subject = issuer = x509.Name([
        x509.NameAttribute(NameOID.COUNTRY_NAME, u"US"),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME, u"MyCompany"),
        x509.NameAttribute(NameOID.COMMON_NAME, f"esp32-{device_id}.local"),
    ])

    cert = x509.CertificateBuilder().subject_name(
        subject
    ).issuer_name(
        issuer
    ).public_key(
        private_key.public_key()
    ).serial_number(
        x509.random_serial_number()
    ).not_valid_before(
        datetime.datetime.utcnow()
    ).not_valid_after(
        # Valid for 10 years
        datetime.datetime.utcnow() + datetime.timedelta(days=3650)
    ).add_extension(
        x509.BasicConstraints(ca=False, path_length=None), critical=True,
    ).sign(private_key, hashes.SHA256())

    # 3. Serialize to PEM format
    key_pem = private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.TraditionalOpenSSL,
        encryption_algorithm=serialization.NoEncryption()
    )

    cert_pem = cert.public_bytes(serialization.Encoding.PEM)

    # Write to temp files
    with open("key.pem", "wb") as f:
        f.write(key_pem)
    with open("cert.pem", "wb") as f:
        f.write(cert_pem)
    
    return key_pem, cert_pem

def create_nvs_csv():
    # Create the CSV required by nvs_partition_gen.py
    # Format: key,type,encoding,value
    csv_content = f"""key,type,encoding,value
{NVS_NAMESPACE},namespace,,
{KEY_NVS_NAME},file,string,key.pem
{CERT_NVS_NAME},file,string,cert.pem
"""
    with open("nvs_data.csv", "w") as f:
        f.write(csv_content)

def generate_nvs_bin():
    # Locate the ESP-IDF tool
    idf_path = os.environ.get('IDF_PATH')
    if not idf_path:
        print("Error: IDF_PATH not set. Please export ESP-IDF environment.")
        return False

    tool_path = os.path.join(idf_path, "components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py")
    
    cmd = [
        sys.executable, 
        tool_path, 
        "generate", 
        "nvs_data.csv", 
        "nvs_prod.bin", 
        NVS_PARTITION_SIZE
    ]

    try:
        subprocess.check_call(cmd)
        print("SUCCESS: nvs_prod.bin generated!")
        return True
    except subprocess.CalledProcessError:
        print("FAILED to generate NVS binary.")
        return False

if __name__ == "__main__":
    # You could pass a Serial Number here
    generate_credentials("SN-12345")
    create_nvs_csv()
    generate_nvs_bin()
    
    print("\nTo flash this to your device:")
    print("esptool.py --port COMx write_flash 0x9000 nvs_prod.bin")
    print("(Check your partitions.csv for the correct address of the NVS partition!)")