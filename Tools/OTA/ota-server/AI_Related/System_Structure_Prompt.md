```text
You are an expert in FastAPI, Python backend development, and embedded IoT firmware update (OTA) architectures. I need your help to configure and modify my Python-based Local Secure OTA Server to make it fully compatible with an ESP32 C++ client.

### 1. Current Project Architecture & Directory Structure
```text
C:.
|   esp32_v1.0.2.bin
|   esp32_v1.0.3.bin
|   generate_certs.py
|   requirements.txt
|   run.py
|   upload_firmware.py
|   
+---certificates
|       ca.crt
|       ca.key
|       server.crt
|       server.key
|       
+---firmware_storage
|       esp32_v1.0.2.bin
|       esp32_v1.0.3.bin
|       manifest.json
|       
\---src
    |   main.py
    |   __init__.py
    |   
    +---api
    |   |   dashboard.py
    |   |   ota.py
    |   |   __init__.py
    |   
    +---core
    |   |   config.py
    |   |   __init__.py
    |   
    +---static
    |   +---css
    |   |       style.css
    |   \---js
    |           main.js
    |           
    +---templates
    |       index.html
```

### 2. Analysis of Existing Code Base

*   **`run.py`**: Inline generator checks and dynamically executes `generate_certs.py` if SSL files are missing, then launches `src/main.py` using a subprocess with `PYTHONPATH` configured to root.
*   **`src/main.py`**: Sets up FastAPI application, validates SSL certificate existences, mounts `/static` for web assets, and registers two routers:
    *   `dashboard_router`: Attached at root `/` (Dashboard tags).
    *   `ota_router`: Attached with prefix `/api/v1/ota` (Device firmware tags).
*   **`src/api/dashboard.py`**: Handles administrative capabilities:
    *   `GET /`: Renders `index.html` displaying current manifest data.
    *   `POST /upload`: Handles multi-part raw binary uploads, writes files to `/firmware_storage/`, calculates file SHA-256 hashes, and safely updates the database inside `manifest.json`.
*   **`src/api/ota.py`**: Implements basic OTA checking and file delivery:
    *   `GET /api/v1/ota/check`: Uses headers `x-ESP32-version` and `x-ESP32-hardware` to check `manifest.json` for updates. If a newer version is found, it returns JSON containing metadata and a dynamically structured download link pointing to `/api/v1/ota/download/{filename}`.
    *   `GET /api/v1/ota/download/{filename}`: Resolves binaries against the dynamic storage target folder and returns them as an `application/octet-stream` via `FileResponse` with path-traversal mitigation checks.

### 3. ESP32 Client Requirements & Current Configuration
My ESP32 uses a custom C++ OTA Client class that enforces specific settings:
*   **Expected File/Download URLs**: Currently configured to request paths like `https://<server_ip>:8443/firmware_storage/firmware_v2.bin` directly, bypassing the dynamic `/api/v1/ota/download/` route structure.
*   **API Security Token Verification**: The client sends custom security headers:
    *   `X-Device-API-Key: secure-device-token-abcde`
*   **Hardware and Version Context Headers**: The client registers its identification context inside these request headers:
    *   `x-ESP32-version`
    *   `x-ESP32-hardware`

### 4. Objective
Provide the necessary modifications to my Python server code (`main.py`, `ota.py`, `dashboard.py`) to bridge the communication mismatch between the ESP32 Client and this FastAPI backend. 

Specifically, address:
1.  How to map raw ESP32 download URLs (targeting `/firmware_storage/{filename}`) into the server's router while still enforcing API-key verification and logging device version headers.
2.  How to update `manifest.json` generation and endpoint structures so they resolve and link paths seamlessly between what the admin panel displays and what the ESP32 client downloads.
3.  Any adjustments to API key and route registrations inside `main.py`.
```