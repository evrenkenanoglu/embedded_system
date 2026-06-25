import os
from pathlib import Path
from pydantic import BaseModel

# Resolves to the 'ota-server' root directory
BASE_DIR = Path(__file__).resolve().parent.parent.parent

class Settings(BaseModel):
    PROJECT_NAME: str = "Local Secure OTA Server"
    
    # Network Bindings
    HOST: str = os.getenv("OTA_HOST", "0.0.0.0")
    PORT: int = int(os.getenv("OTA_PORT", 8443))
    
    # Directory Configurations
    CERT_DIR: Path = BASE_DIR / "certificates"
    FIRMWARE_DIR: Path = BASE_DIR / "firmware_storage"
    TEMPLATES_DIR: Path = BASE_DIR / "src" / "templates"
    STATIC_DIR: Path = BASE_DIR / "src" / "static"
    
    # Cryptographic File Paths
    SSL_CERT_FILE: Path = CERT_DIR / "server.crt"
    SSL_KEY_FILE: Path = CERT_DIR / "server.key"
    CA_CERT_FILE: Path = CERT_DIR / "ca.crt"
    
    # Manifest Configuration
    MANIFEST_FILE: Path = FIRMWARE_DIR / "manifest.json"
    
    # Client Security Verification (Optional Header Token verification)
    API_KEY_HEADER: str = "X-Device-API-Key"
    API_KEY: str = os.getenv("OTA_API_KEY", "secure-device-token-abcde")

    # Platform-Independent Device Identification Headers
    HEADER_VERSION_KEY: str = os.getenv("OTA_HEADER_VERSION", "x-ESP32-version")
    HEADER_HARDWARE_KEY: str = os.getenv("OTA_HEADER_HARDWARE", "x-ESP32-hardware")

    # System Configuration Defaults
    FIRMWARE_EXTENSION: str = os.getenv("OTA_FIRMWARE_EXT", ".bin")
    DEFAULT_MIN_LOADER_VERSION: str = "1.0.0"
    
    # Network I/O Buffer Parameters
    CHUNK_SIZE_BYTES: int = int(os.getenv("OTA_CHUNK_SIZE", 8192))

settings = Settings()