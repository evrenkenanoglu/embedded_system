```text
You are an expert in FastAPI, Python backend development, and embedded IoT firmware update (OTA) architectures. I need your help to maintain, scale, or troubleshoot my Python-based Local Secure OTA Server, which is fully integrated with a platform-independent, polymorphic C++ ESP32 client.

### 1. Current Project Architecture & Directory Structure
```text
C:.
|   generate_certs.py
|   requirements.txt
|   run.py
|   upload_firmware.py
|   
+---AI_Related
|       System_Structure_Prompt.md
|       
+---bin
|       detools.exe                   # Standalone Windows Binary (Zero-Install)
|       detools-linux                 # Standalone Linux Binary (Zero-Install)
|       detools-macos                 # Standalone macOS Binary (Zero-Install)
|       
+---certificates
|       ca.crt                        # Client-side Trust Anchor
|       ca.key
|       server.crt
|       server.key
|       
+---firmware_storage
|   |   Embedded_IoT_BT_WIFI_Base_Project.bin
|   |   esp32_v1.0.2.bin
|   |   esp32_v1.0.3.bin
|   |   manifest.json                 # Core Metadata Database
|   |   
|   \---patches
|           patch_1.0.0_to_1.0.1.bin  # Pre-generated binary diff patches
|           
\---src
    |   main.py
    |   __init__.py
    |   
    +---api
    |   |   dashboard.py
    |   |   ota.py
    |   |   __init__.py
    |   |   
    |   \---__pycache__
    |           dashboard.cpython-313.pyc
    |           ota.cpython-313.pyc
    |           __init__.cpython-313.pyc
    |           
    +---core
    |   |   config.py
    |   |   patch.py                  # Portable Subprocess-based Patch Generator
    |   |   __init__.py
    |   |   
    |   \---__pycache__
    |           config.cpython-313.pyc
    |           patch.cpython-313.pyc
    |           __init__.cpython-313.pyc
    |           
    +---static
    |   +---css
    |   |       style.css
    |   |       
    |   \---js
    |           main.js
    |           
    +---templates
    |       index.html
    |       
    \---__pycache__
            main.cpython-313.pyc
            __init__.cpython-313.pyc
```

### 2. Analysis of Existing Code Base

*   **`run.py`**: Asserts dynamic certificate generation via `generate_certs.py` and boots `src/main.py` using a subprocess with configured environment variables.
*   **`src/main.py`**: Initializes the FastAPI instance, mounts `/static` for administrative dashboard styling, auto-generates dynamic subdirectories (`patches/`), and exposes `/api/v1/ota` plus root paths.
*   **`src/core/patch.py`**: Encapsulates a cross-platform, compile-free, zero-installation patching tool. It uses `subprocess` to call the standalone `detools` binary in `/bin/` depending on the host OS, eliminating runtime virtual environment compilation errors on Windows/macOS.
*   **`src/api/dashboard.py`**: Handles firmware uploads. Locates the previous compiled binary, calls the modular patch engine to compute an compressed binary diff (`heatshrink` algorithm), and registers both the full binary and delta patch metadata inside `manifest.json`.
*   **`src/api/ota.py`**: Exposes the dynamic update check (`/api/v1/ota/check`) and secure, time-limited presigned download endpoints (e.g., `/firmware_storage/{filename:path}?token=...`):
    *   If a pre-calculated delta patch matches the client's current version, it delivers a signed download link directly targeting the delta patch file and sets `"update_type": "delta"`.
    *   Otherwise, it dynamically falls back to delivering the complete binary and sets `"update_type": "full"`.
    *   It uses SHA-256 HMAC-signed tokens valid for 300 seconds to restrict direct access to binaries.

### 3. ESP32 Client Architecture & Configuration
My ESP32 uses a custom C++ OTA Client designed under strict constraints (`-fno-exceptions` and `-fno-rtd` environments):
*   **Platform-Independent `OtaManager` (Application Layer)**: Completely decoupled from target SoC APIs. It manages high-level orchestration, accepting configuration profiles and executing:
    *   **The Jitter Rule**: Adding random timing offsets to periodic checks via custom RNG hooks to balance server load.
    *   **The Two-Stage Handshake**: Separating Stage 1 Checks from Stage 2 Downloads. It postpones streaming until platform power status (battery > 80%) and local RTC time constraints (e.g., low-traffic hours at 2:00 AM) are satisfied.
*   **Polymorphic `mem_ota` (HAL / Memory Layer)**:
    *   Implements the abstract `IHal_Mem_Ota` interface.
    *   Bypasses typical compilation spaghetti by handling both full binary writes (`esp_ota_write`) and decompressed streaming patching (`esp_delta_ota` API config structure with C-callbacks) internally.
    *   **Header Byte Accumulator**: Features an internal 320-byte accumulator to cache incoming micro-sized stream blocks (e.g., 128 bytes) until they reach the minimum required size (288 bytes) for ESP-IDF application header validation before writing blocks to flash memory.
*   **Secure Validation Rules**:
    *   Client uses `ca.crt` content as the Trust Anchor, with certificate configuration parameters (`server_cert_len`) explicitly set to `0` to ensure PEM-encoded string parsing instead of raw binary DER parsing.

### 4. Objective
Provide support, architectural guidance, expansion features, or troubleshooting steps for this complete client-server framework. 

Specifically, address:
1.  How to scale the server to support canary/staggered rollouts to specific device IDs using the current `manifest.json` structure.
2.  How to implement cryptographically secure client-side signing (ECDSA SHA-256) on the server-side to let the ESP32 verify binary origins.
3.  Any debugging assistance regarding socket timeouts, token expirations, or flash memory write configurations.
```