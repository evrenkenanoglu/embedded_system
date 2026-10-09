import sys
import socket
import uvicorn
from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles

from src.api.ota import router as ota_router, direct_router as direct_ota_router
from src.api.dashboard import router as dashboard_router
from src.core.config import settings

app = FastAPI(title=settings.PROJECT_NAME)

# Ensure runtime directories exist
settings.STATIC_DIR.mkdir(parents=True, exist_ok=True)
settings.TEMPLATES_DIR.mkdir(parents=True, exist_ok=True)
settings.CATALOG_DIR.mkdir(parents=True, exist_ok=True)
settings.BINARIES_DIR.mkdir(parents=True, exist_ok=True)
settings.PATCHES_DIR.mkdir(parents=True, exist_ok=True)

# Mount static files
app.mount("/static", StaticFiles(directory=str(settings.STATIC_DIR)), name="static")

# Register routes
app.include_router(dashboard_router, tags=["Administrative Dashboard"])
app.include_router(ota_router, prefix="/api/v1/ota", tags=["Device Firmware Updates"])
app.include_router(direct_ota_router, tags=["Device Direct Firmware Updates"])


def verify_ssl_credentials():
    """Validates that TLS HTTPS transport certificates exist. (Zero private code-signing keys required)."""
    cert_exists = settings.SSL_CERT_FILE.exists()
    key_exists = settings.SSL_KEY_FILE.exists()

    if not cert_exists or not key_exists:
        print("ERROR: SSL/TLS transport credentials are missing.")
        print(f"  Certificate Path: {settings.SSL_CERT_FILE} (Found: {cert_exists})")
        print(f"  Key Path:         {settings.SSL_KEY_FILE} (Found: {key_exists})")
        print("Run 'inv es.crypto.generate-pki' to generate certificates.")
        sys.exit(1)


def get_local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("8.8.8.8", 80))
        local_ip = s.getsockname()[0]
    except Exception:
        local_ip = "127.0.0.1"
    finally:
        s.close()
    return local_ip


if __name__ == "__main__":
    verify_ssl_credentials()

    bind_host = settings.HOST if settings.HOST != "0.0.0.0" else "localhost"
    print(f"Starting Secure OTA Server at https://{bind_host}:{settings.PORT}")
    print(f"Local IP Access Endpoint:   https://{get_local_ip()}:{settings.PORT}")
    print(f"Serving Release Catalog:    {settings.CATALOG_DIR}")

    uvicorn.run(
        "main:app",
        host=settings.HOST,
        port=settings.PORT,
        ssl_keyfile=str(settings.SSL_KEY_FILE),
        ssl_certfile=str(settings.SSL_CERT_FILE),
        reload=settings.RELOAD,
    )
