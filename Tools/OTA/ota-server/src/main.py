import sys
from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles
from src.api.ota import router as ota_router, direct_router as direct_ota_router
from src.api.dashboard import router as dashboard_router
from src.core.config import settings
import uvicorn
import socket

app = FastAPI(title=settings.PROJECT_NAME)

# Ensure required runtime directories exist
settings.STATIC_DIR.mkdir(parents=True, exist_ok=True)
settings.TEMPLATES_DIR.mkdir(parents=True, exist_ok=True)
settings.FIRMWARE_DIR.mkdir(parents=True, exist_ok=True)
(settings.FIRMWARE_DIR / "patches").mkdir(parents=True, exist_ok=True) # Create patches subdir

# Mount static assets (CSS, JS, Images) for the dashboard UI
app.mount("/static", StaticFiles(directory=str(settings.STATIC_DIR)), name="static")

# Register system routes
app.include_router(dashboard_router, tags=["Administrative Dashboard"])
app.include_router(ota_router, prefix="/api/v1/ota", tags=["Device Firmware Updates"])
app.include_router(direct_ota_router, tags=["Device Direct Firmware Updates"]) # Handles root /firmware_storage/ paths


def verify_ssl_credentials():
    """Asserts that SSL certificates are generated and present before server boot."""
    cert_exists = settings.SSL_CERT_FILE.exists()
    key_exists = settings.SSL_KEY_FILE.exists()

    if not cert_exists or not key_exists:
        print("ERROR: Cryptographic SSL infrastructure files are missing.")
        print(f"  Target Certificate Path: {settings.SSL_CERT_FILE} (Found: {cert_exists})")
        print(f"  Target Private Key Path: {settings.SSL_KEY_FILE} (Found: {key_exists})")
        print("ACTION REQUIRED: Execute 'python generate_certs.py' to create local certificates.")
        sys.exit(1)


def get_local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # 8.8.8.8 is Google's public DNS server. The connection is not actually completed.
        s.connect(("8.8.8.8", 80))
        local_ip = s.getsockname()[0]
    except Exception:
        local_ip = "127.0.0.1" # Fallback to loopback if not connected to any network
    finally:
        s.close()
    return local_ip


if __name__ == "__main__":
    verify_ssl_credentials()

    # Local IP logs
    print(f"Starting Secure Local OTA Server at https://{settings.HOST if settings.HOST != '0.0.0.0' else 'localhost'}:{settings.PORT}")
    print(f"Starting Secure Local OTA Server at https://{get_local_ip()}:{settings.PORT}")
    
    uvicorn.run(
        "main:app",
        host=settings.HOST,
        port=settings.PORT,
        ssl_keyfile=str(settings.SSL_KEY_FILE),
        ssl_certfile=str(settings.SSL_CERT_FILE),
        reload=True  # Enabled for active development monitoring
    )