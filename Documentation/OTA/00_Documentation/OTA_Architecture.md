# OTA Update Framework Architecture Documentation

This document describes the design, routing mechanisms, security models, and end-to-end execution flows of the modular, platform-independent Over-The-Air (OTA) update framework.

---

## 1. System Architectural Layers (Client-Side)

The client framework is split into distinct abstraction layers to remain platform-agnostic, separating high-level business rules from platform-specific hardware operations.

```

+-------------------------------------------------------------------+
|                         Application Layer                         |
|                 (IOtaManager / Concrete OtaManager)               |
+-------------------------------------------------------------------+
                                  |
                                  v
+-------------------------------------------------------------------+
|                        PAL / Service Layer                        |
|                 (IOtaService / Concrete OtaService)               |
+-------------------------------------------------------------------+
             |                                         |
             v                                         v
+-------------------------+               +---------------------------+
|     PAL / Transport     |               |  HAL / Memory Abstraction |
|   (IOtaTransport /      |               |  (IHal_Mem_Ota /          |
|    OtaHttpTransport /   |               |   mem_ota ESP32 Wrapper)  |
|    IHttpClient)         |               +---------------------------+
+-------------------------+

```

### HAL (Hardware Abstraction Layer)
*   **Interfaces**: `IHal_Mem_Ota`
*   **Concrete Drivers**: `mem_ota` (ESP32 `esp_ota_ops` driver wrapper)
*   **Role**: Manages physical partitions, raw memory allocation, binary writes, verification, and active boot partition updates. It abstracts sector-level actions entirely from upper layers.

### PAL / Transport Layer
*   **Interfaces**: `IHttpClient` (Generic HTTP), `IOtaTransport` (Protocol-agnostic stream adapter)
*   **Concrete Drivers**: `HttpsClient` (ESP-IDF HTTP client), `OtaHttpTransport` (URL parser and stream controller)
*   **Role**: Handles lower-level TCP sockets and TLS context validation. Translates high-level connection instructions into chunked payloads and forwards standard protocol streams to the layer above.

### PAL / Service Layer
*   **Interfaces**: `IOtaService`
*   **Concrete Coordinator**: `OtaService`
*   **Role**: Coordinates the core OTA state machine (`Idle`, `Downloading`, `Verifying`, `Applying`, `Success`, `Failed`). It reads chunked network streams from `IOtaTransport` and writes them into `IHal_Mem_Ota`.

### Application Layer
*   **Interfaces**: `IOtaManager`
*   **Concrete Controller**: `OtaManager`
*   **Role**: Manages high-level decisions, such as checking for update manifests, validating system power states, comparing firmware semantic versions (SemVer), evaluating local anti-downgrade (HSVN), and initiating system reboots after success.

---

## 2. Server-Side Routing & Architecture

The server is a generic FastAPI implementation configured to serve as a secure local OTA distribution point. It isolates raw binary endpoints and relies on dynamic parameters derived from a central configuration file (`src/core/config.py`).

### Active Backend Endpoints
All download-related routes are dynamically bound to the physical directory name mapped in `settings.FIRMWARE_DIR` (e.g. `/firmware_storage`).

| Method   | Route                             | Description                                 | Auth Required           |
| :------- | :--------------------------------- | :------------------------------------------ | :---------------------- |
| **GET**  | `/`                               | Web Administrative Dashboard UI             | None (Browser)          |
| **POST** | `/upload`                         | Dashboard form upload for new binaries      | None (Browser)          |
| **GET**  | `/api/v1/ota/check`               | Evaluates hardware and version availability | Dynamic HTTP Header Key |
| **POST** | `/api/v1/ota/status`              | Logs success/failure telemetry reports      | Dynamic HTTP Header Key |
| **GET**  | `/api/v1/ota/download/{filename}` | Serves binaries using standard api prefix   | Dynamic Header OR Token |
| **GET**  | `/<FIRMWARE_DIR>/{filename}`      | Root-level dynamic binary downloader        | Dynamic Header OR Token |

### Security & Operational Implementations

*   **API Security Key Validation**: Clients must submit an authentication token via the header defined in `settings.API_KEY_HEADER` (e.g., `X-Device-API-Key: secure-device-token-abcde`).
*   **Anti-Downgrade HSVN Verification**: The target client sends its local Hardware Security Version Number (HSVN) via the header defined in `settings.HEADER_HSVN_KEY`. The server verifies that the client's current HSVN is not higher than the target firmware's HSVN to prevent unauthorized firmware rollbacks.
*   **Canary Rollout Orchestration**: The `/check` endpoint uses a deterministic MD5 hash of the requesting device ID combined with the target version to verify if a device falls within the defined rollout percentile (`canary_percentage`), avoiding database lookups for state.
*   **Automated Telemetry & Rollback Guardrails**: Devices invoke `/status` post-update. The server calculates failure rates dynamically. If the failed reports cross `settings.MAX_FAILURE_RATE_PERCENT` over a threshold of reports, the target version status is marked as `soft-rolled-back`, and update inquiries fall back automatically.
*   **Presigned Download Token Engine**: Direct file access routes require an HMAC SHA-256 signature passed as a query parameter (`token=`).
    *   **Generation**: On a successful `/check` query, the server calculates a UNIX timestamp for expiration (default: +300 seconds) and generates a signature matching `sha256_hmac(key=API_KEY, msg="filename:expiration")`.
    *   **Validation**: The download endpoint recreates the signature and enforces a strict expiration window.
*   **Directory Traversal Protection**: All file requests are evaluated with path-resolution validation to ensure no requests escape the defined boundaries of `settings.FIRMWARE_DIR`:
    ```python
    file_path = (settings.FIRMWARE_DIR / filename).resolve()
    if not file_path.is_relative_to(settings.FIRMWARE_DIR.resolve()):
        raise HTTPException(status_code=403, detail="Access denied.")
    ```

---

## 3. End-to-End Execution Sequence

This sequence diagram illustrates a secure dual-phase update cycle where the client performs identity verification, runs safety and rollout gates, acquires a transient download token, downloads the stream, and logs the execution outcome.

```
[ ESP32 Client ]                                  [ FastAPI Host ]                       [ Local Disk / Storage ]
       |                                                 |                                          |
       | 1. Query Check Request                          |                                          |
       |    Headers:                                     |                                          |
       |      X-Device-API-Key: token                    |                                          |
       |      x-ESP32-version: 1.0.0                     |                                          |
       |      x-ESP32-hardware: S3-WROOM                 |                                          |
       |      x-ESP32-device-id: MAC_ADDR                |                                          |
       |      x-ESP32-channel: stable                    |                                          |
       |      x-ESP32-hsvn: 1                            |                                          |
       |------------------------------------------------>|                                          |
       |                                                 | 2. Check update against manifest.json    |
       |                                                 |----------------------------------------->|
       |                                                 |<-----------------------------------------|
       |                                                 |                                          |
       |                                                 | 3. If update available:                  |
       |                                                 |    - Run HSVN evaluation                 |
       |                                                 |    - Run canary percentage check         |
       |                                                 |    - Generate:                           |
       |                                                 |       - Expiration: current_time + 300s  |
       |                                                 |       - Token: hmac(file + expiration)   |
       |                                                 |                                          |
       | 4. Returns JSON Update Payload                  |                                          |
       |    - Update: True                               |                                          |
       |    - SHA-256: 0xAbCd...                         |                                          |
       |    - URL: https://ip:port/storage/bin?token=t   |                                          |
       |<------------------------------------------------|                                          |
       |                                                 |                                          |
       | ========================================================================================== |
       |                                 PHASE 2: SECURE STREAM DOWNLOAD                            |
       | ========================================================================================== |
       |                                                 |                                          |
       | 5. GET /firmware_storage/esp32_v1.0.3.bin?token |                                          |
       |------------------------------------------------>|                                          |
       |                                                 | 6. Validate token expiration & signature |
       |                                                 |                                          |
       |                                                 | 7. Read binary file                      |
       |                                                 |----------------------------------------->|
       |                                                 |<-----------------------------------------|
       |                                                 |                                          |
       | 8. Streams binary payload in chunks             |                                          |
       |<------------------------------------------------|                                          |
       |                                                 |                                          |
       | 9. Write bytes to flash memory via partition    |                                          |
       |    API and calculate running SHA-256             |                                          |
       |--+                                              |                                          |
       |  |                                              |                                          |
       |<-+                                              |                                          |
       |                                                 |                                          |
       | 10. Perform post-write validation of SHA-256    |                                          |
       |     against check-phase hash. On success,       |                                          |
       |     switch boot flag and restart.               |                                          |
       |--+                                              |                                          |
       |  |                                              |                                          |
       |<-+                                              |                                          |
       |                                                 |                                          |
       | ========================================================================================== |
       |                                 PHASE 3: TELEMETRY REPORTING                               |
       | ========================================================================================== |
       |                                                 |                                          |
       | 11. POST /api/v1/ota/status                     |                                          |
       |     Payload: {device_id, status: "success",...} |                                          |
       |------------------------------------------------>|                                          |
       |                                                 | 12. Evaluate error metrics per version   |
       |                                                 |     and apply automatic rolling rollback  |
       |                                                 |     to manifest if threshold is breached. |
       |                                                 |--+                                       |
       |                                                 |  |                                       |
       |                                                 |<-+                                       |
```

---

## 4. Key Client-Side Implementation Fixes

### Exception Safety (`-fno-exceptions`)
Standard-library components that throw exceptions under memory limits or invalid parsing were removed. 
*   **URL Parsing**: Uses non-throwing `std::strtol` with strict `endptr` checking to validate port values.
*   **Out-Of-Memory Handling**: Structural operations (such as appending HTTP event data chunks to buffers via `std::vector::insert`) execute standard system termination handlers natively if a physical heap allocation failure occurs.

### Dynamic Port & TLS Scheme Matching
The client uses the boolean `outIsHttps` output directly to establish TLS handshakes, supporting custom secure ports (e.g. `8443`) regardless of the port value itself.

### ESP-IDF Configuration Path Compliance
The initialization parameters are populated with a default fallback path of `"/"`. The actual URI target is modified dynamically before transmitting requests via `sendRequest()`.
