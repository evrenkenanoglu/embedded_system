import os
import sys
from pathlib import Path
from typing import Any, Dict, List
import yaml

# Base directory: Tools/OTA/ota-server
BASE_DIR = Path(__file__).resolve().parent.parent.parent


def resolve_config_file() -> Path:
    """Resolves configuration file from environment variable, CLI args, or workspace cascade."""
    # 1. Environment variable (set by run.py or task runner)
    env_path = os.environ.get("OTA_CONFIG_PATH")
    if env_path:
        cand = Path(env_path)
        resolved = cand if cand.is_absolute() else (Path.cwd() / cand).resolve()
        if resolved.exists():
            return resolved

    # 2. Explicit CLI argument (--config <path> / -c <path>)
    for idx, arg in enumerate(sys.argv[:-1]):
        if arg in ("--config", "-c"):
            cand = Path(sys.argv[idx + 1])
            resolved = cand if cand.is_absolute() else (Path.cwd() / cand).resolve()
            if resolved.exists():
                os.environ["OTA_CONFIG_PATH"] = str(resolved)
                return resolved

    # 3. Upward search for configs/config_ota_server.yaml from current location
    curr = BASE_DIR
    while curr != curr.parent:
        cand = curr / "configs" / "config_ota_server.yaml"
        if cand.exists():
            resolved = cand.resolve()
            os.environ["OTA_CONFIG_PATH"] = str(resolved)
            return resolved
        curr = curr.parent

    # 4. Local tool fallbacks
    for cand in [BASE_DIR / "config.yaml", BASE_DIR / "config.example.yaml"]:
        if cand.exists():
            return cand.resolve()

    return (BASE_DIR / "config.yaml").resolve()


CONFIG_FILE = resolve_config_file()


def _expand_placeholders(data: Any, context: Dict[str, str]) -> Any:
    """Replaces placeholders like {project_root_dir} and {server_root_dir}."""
    if isinstance(data, dict):
        return {k: _expand_placeholders(v, context) for k, v in data.items()}
    elif isinstance(data, list):
        return [_expand_placeholders(item, context) for item in data]
    elif isinstance(data, str):
        for key, val in context.items():
            data = data.replace(f"{{{key}}}", str(val))
        return data
    return data


def _load_and_resolve_yaml() -> dict:
    if not CONFIG_FILE.exists():
        print(f"ERROR: Configuration file not found at '{CONFIG_FILE}'")
        sys.exit(1)

    with open(CONFIG_FILE, "r", encoding="utf-8") as f:
        try:
            raw = yaml.safe_load(f) or {}
        except yaml.YAMLError as exc:
            print(f"ERROR: Failed to parse '{CONFIG_FILE}': {exc}")
            sys.exit(1)

    env_cfg = raw.get("environment", {})

    # 1. Resolve project_root_dir
    project_root_raw = env_cfg.get("project_root_dir", ".")
    project_root = str(Path(project_root_raw).resolve())

    # 2. Resolve server_root_dir
    server_root_raw = env_cfg.get("server_root_dir", str(BASE_DIR))
    server_root_raw = server_root_raw.replace("{project_root_dir}", project_root)
    server_root = str(Path(server_root_raw).resolve())

    context = {
        "project_root_dir": project_root,
        "server_root_dir": server_root,
    }

    return _expand_placeholders(raw, context)


_cfg = _load_and_resolve_yaml()


class Settings:
    _env = _cfg.get("environment", {})
    PROJECT_ROOT_DIR: Path = Path(_env.get("project_root_dir", BASE_DIR))
    SERVER_ROOT_DIR: Path = Path(_env.get("server_root_dir", BASE_DIR))

    _server = _cfg.get("server", {})
    PROJECT_NAME: str = _server.get("project_name", "Local Secure OTA Server")
    HOST: str = _server.get("host", "0.0.0.0")
    PORT: int = int(_server.get("port", 8443))
    RELOAD: bool = bool(_server.get("reload", False))

    _paths = _cfg.get("paths", {})
    CERT_DIR: Path = Path(_paths.get("cert_dir", SERVER_ROOT_DIR / "certs")).resolve()
    FIRMWARE_DIR: Path = Path(
        _paths.get("firmware_dir", SERVER_ROOT_DIR / "firmware_storage")
    ).resolve()
    TEMPLATES_DIR: Path = Path(
        _paths.get("templates_dir", SERVER_ROOT_DIR / "src/templates")
    ).resolve()
    STATIC_DIR: Path = Path(
        _paths.get("static_dir", SERVER_ROOT_DIR / "src/static")
    ).resolve()
    TELEMETRY_LOG_DIR: Path = Path(
        _paths.get("telemetry_dir", SERVER_ROOT_DIR / "telemetry_logs")
    ).resolve()
    MANIFEST_FILE: Path = (
        FIRMWARE_DIR / _paths.get("manifest_filename", "manifest.json")
    ).resolve()

    _certs = _cfg.get("certificates", {})
    _ca_cfg = _certs.get("ca", {})
    _srv_cfg = _certs.get("server", {})
    _sign_cfg = _certs.get("signing", {})

    CA_COMMON_NAME: str = _ca_cfg.get("common_name", "MasterRootCA")
    CA_KEY_TYPE: str = _ca_cfg.get("key_type", "ec-secp256r1")
    CA_KEY_SIZE: int = int(_ca_cfg.get("key_size", 256))
    CA_VALIDITY_DAYS: int = int(_ca_cfg.get("validity_days", 3650))
    CA_CERT_FILE: Path = (CERT_DIR / _ca_cfg.get("cert_file", "ca.crt")).resolve()
    CA_KEY_FILE: Path = (CERT_DIR / _ca_cfg.get("key_file", "ca.key")).resolve()

    SERVER_COMMON_NAME: str = _srv_cfg.get("common_name", "localhost")
    SERVER_KEY_TYPE: str = _srv_cfg.get("key_type", "ec-secp256r1")
    SERVER_KEY_SIZE: int = int(_srv_cfg.get("key_size", 256))
    SERVER_VALIDITY_DAYS: int = int(_srv_cfg.get("validity_days", 365))
    SSL_CERT_FILE: Path = (CERT_DIR / _srv_cfg.get("cert_file", "server.crt")).resolve()
    SSL_KEY_FILE: Path = (CERT_DIR / _srv_cfg.get("key_file", "server.key")).resolve()
    STATIC_DNS_SANS: List[str] = _srv_cfg.get("dns_sans", ["localhost", "ota.local"])
    STATIC_IP_SANS: List[str] = _srv_cfg.get("ip_sans", ["127.0.0.1", "192.168.1.100"])

    SIGNING_CERT_COMMON_NAME: str = _sign_cfg.get(
        "common_name", "DeveloperFirmwareSigning"
    )
    SIGNING_CERT_ORG: str = _sign_cfg.get("organization", "Firmware Release Authority")
    SIGNING_KEY_TYPE: str = _sign_cfg.get("key_type", "ec-secp256r1")
    SIGNING_VALIDITY_DAYS: int = int(_sign_cfg.get("validity_days", 365))
    SIGNING_CRT_FILE: Path = (
        CERT_DIR / _sign_cfg.get("cert_file", "signing.crt")
    ).resolve()
    SIGNING_KEY_FILE: Path = (
        CERT_DIR / _sign_cfg.get("key_file", "signing.key")
    ).resolve()

    _auth = _cfg.get("auth", {})
    API_KEY_HEADER: str = _auth.get("api_key_header", "X-Device-API-Key")
    API_KEY: str = _auth.get("api_key", "secure-device-token-factory-001")
    TOKEN_EXPIRATION_SECONDS: int = int(_auth.get("token_expiration_seconds", 300))

    _headers = _cfg.get("client_headers", {})
    HEADER_VERSION_KEY: str = _headers.get("version", "x-ESP32-version")
    HEADER_HARDWARE_KEY: str = _headers.get("hardware", "x-ESP32-hardware")
    HEADER_DEVICE_ID_KEY: str = _headers.get("device_id", "x-ESP32-device-id")
    HEADER_CHANNEL_KEY: str = _headers.get("channel", "x-ESP32-channel")
    HEADER_HSVN_KEY: str = _headers.get("hsvn", "x-ESP32-hsvn")

    _deploy = _cfg.get("deployment", {})
    DEFAULT_CHANNEL: str = _deploy.get("default_channel", "stable")
    DEFAULT_MIN_LOADER_VERSION: str = _deploy.get("default_min_loader_version", "1.0.0")
    DEFAULT_HSVN: int = int(_deploy.get("default_hsvn", 1))
    DEFAULT_CANARY_PERCENTAGE: int = int(_deploy.get("default_canary_percentage", 100))
    FIRMWARE_EXTENSION: str = _deploy.get("firmware_extension", ".bin")
    CHUNK_SIZE_BYTES: int = int(_deploy.get("chunk_size_bytes", 8192))

    _rollback = _cfg.get("rollback", {})
    MAX_FAILURE_RATE_PERCENT: float = float(
        _rollback.get("max_failure_rate_percent", 10.0)
    )
    MIN_STATUS_REPORTS_FOR_ROLLBACK: int = int(
        _rollback.get("min_reports_for_rollback", 5)
    )


settings = Settings()
