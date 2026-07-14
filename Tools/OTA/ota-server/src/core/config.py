import os
from pathlib import Path
from pydantic_settings import BaseSettings

# Resolves to the 'ota-server' root directory
BASE_DIR = Path(__file__).resolve().parent.parent.parent

class Settings(BaseSettings):
    PROJECT_NAME: str = os.getenv("OTA_PROJECT_NAME", "Local Secure OTA Server")
    
    # Network Bindings
    HOST: str = os.getenv("OTA_HOST", "0.0.0.0")
    PORT: int = int(os.getenv("OTA_PORT", "8443"))
    
    # Directory Configurations
    CERT_DIR: Path = BASE_DIR / os.getenv("OTA_CERT_DIR", "certificates")
    FIRMWARE_DIR: Path = BASE_DIR / os.getenv("OTA_FIRMWARE_DIR", "firmware_storage")
    TEMPLATES_DIR: Path = BASE_DIR / os.getenv("OTA_TEMPLATES_DIR", "src/templates")
    STATIC_DIR: Path = BASE_DIR / os.getenv("OTA_STATIC_DIR", "src/static")
    TELEMETRY_LOG_DIR: Path = BASE_DIR / os.getenv("OTA_TELEMETRY_DIR", "telemetry_logs")
    
    # Cryptographic File Paths
    SSL_CERT_FILE: Path = CERT_DIR / os.getenv("OTA_SSL_CERT_NAME", "server.crt")
    SSL_KEY_FILE: Path = CERT_DIR / os.getenv("OTA_SSL_KEY_NAME", "server.key")
    CA_CERT_FILE: Path = CERT_DIR / os.getenv("OTA_CA_CERT_NAME", "ca.crt")
    SIGNING_KEY_FILE: Path = CERT_DIR / os.getenv("OTA_SIGNING_KEY_NAME", "signing.key")
    SIGNING_CRT_FILE: Path = CERT_DIR / os.getenv("OTA_SIGNING_CRT_NAME", "signing.crt")
    
    # Manifest Configuration
    MANIFEST_FILE: Path = FIRMWARE_DIR / os.getenv("OTA_MANIFEST_NAME", "manifest.json")
    
    # Client Security Verification (Authentication Token Settings)
    API_KEY_HEADER: str = os.getenv("OTA_API_KEY_HEADER", "X-Device-API-Key")
    API_KEY: str = os.getenv("OTA_API_KEY", "secure-device-token-abcde")

    # Platform-Independent Device Identification Headers
    HEADER_VERSION_KEY: str = os.getenv("OTA_HEADER_VERSION", "x-ESP32-version")
    HEADER_HARDWARE_KEY: str = os.getenv("OTA_HEADER_HARDWARE", "x-ESP32-hardware")
    HEADER_DEVICE_ID_KEY: str = os.getenv("OTA_HEADER_DEVICE_ID", "x-ESP32-device-id")
    HEADER_CHANNEL_KEY: str = os.getenv("OTA_HEADER_CHANNEL", "x-ESP32-channel")
    HEADER_HSVN_KEY: str = os.getenv("OTA_HEADER_HSVN", "x-ESP32-hsvn")

    # Code Signing Parameters
    SIGNING_CERT_COMMON_NAME: str = os.getenv("OTA_SIGNING_CN", "Firmware Code Signing Cert")
    SIGNING_CERT_ORG: str = os.getenv("OTA_SIGNING_O", "Enterprise IoT")
    SIGNING_VALIDITY_DAYS: int = int(os.getenv("OTA_SIGNING_DAYS", "365"))
    
    # Dynamic Rollout & Canary Defaults
    DEFAULT_CHANNEL: str = os.getenv("OTA_DEFAULT_CHANNEL", "stable")
    DEFAULT_MIN_LOADER_VERSION: str = os.getenv("OTA_DEFAULT_MIN_LOADER", "1.0.0")
    DEFAULT_HSVN: int = int(os.getenv("OTA_DEFAULT_HSVN", "1"))
    DEFAULT_CANARY_PERCENTAGE: int = int(os.getenv("OTA_DEFAULT_CANARY", "100"))
    TOKEN_EXPIRATION_SECONDS: int = int(os.getenv("OTA_TOKEN_EXPIRY", "300"))
    
    # Network I/O Buffer Parameters
    CHUNK_SIZE_BYTES: int = int(os.getenv("OTA_CHUNK_SIZE", "8192"))
    FIRMWARE_EXTENSION: str = os.getenv("OTA_FIRMWARE_EXT", ".bin")

    # Auto-Rollback Safety Limits
    MAX_FAILURE_RATE_PERCENT: float = float(os.getenv("OTA_MAX_FAILURE_RATE", "10.0"))
    MIN_STATUS_REPORTS_FOR_ROLLBACK: int = int(os.getenv("OTA_MIN_REPORTS", "5"))

    class Config:
        case_sensitive = True

settings = Settings()