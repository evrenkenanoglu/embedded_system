# OTA Update Framework Architecture Documentation

This document describes the design, routing mechanisms, security models, and end-to-end execution flows of the modular, platform-independent Over-The-Air (OTA) update framework. The system is architected as a decoupled client-server pattern optimized for secure, resilient embedded applications.

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
*   **Role**: Manages high-level decisions, such as checking for update manifests, validating system power states, comparing firmware semantic versions (SemVer), and initiating system reboots after success.

---

## 2. Server-Side Routing & Architecture

The server is a generic FastAPI implementation configured to serve as a secure local OTA distribution point. It isolates raw binary endpoints and relies on dynamic parameters derived from a central configuration file (`src/core/config.py`).

### Active Backend Endpoints
All download-related routes are dynamically bound to the physical directory name mapped in `settings.FIRMWARE_DIR` (e.g. `/firmware_storage`).

| Method   | Route                             | Description                                 | Auth Required           |
| :------- | :-------------------------------- | :------------------------------------------ | :---------------------- |
| **GET**  | `/`                               | Web Administrative Dashboard UI             | None (Browser)          |
| **POST** | `/upload`                         | Dashboard form upload for new binaries      | None (Browser)          |
| **GET**  | `/api/v1/ota/check`               | Evaluates hardware and version availability | Dynamic HTTP Header Key |
| **GET**  | `/api/v1/ota/download/{filename}` | Serves binaries using standard api prefix   | Dynamic Header OR Token |
| **GET**  | `/<FIRMWARE_DIR>/{filename}`      | Root-level dynamic binary downloader        | Dynamic Header OR Token |

### Security Implementations

*   **API Security Key Validation**: Clients must submit an authentication token via the header defined in `settings.API_KEY_HEADER` (e.g., `X-Device-API-Key: secure-device-token-abcde`).
*   **Presigned Download Token Engine**: Direct file access routes (`/firmware_storage/{filename}`) require an HMAC SHA-256 signature passed as a query parameter (`token=`).
    *   **Generation**: On a successful `/check` query, the server calculates a UNIX timestamp for expiration (default: +300 seconds) and generates a signature matching `sha256_hmac(key=API_KEY, msg="filename:expiration")`.
    *   **Validation**: The download endpoint recreates the signature and enforces a strict expiration window. This mimics AWS S3 Presigned URLs and prevents unauthorized, persistent file access.
*   **Directory Traversal Protection**: All file requests are evaluated with path-resolution validation to ensure no requests escape the defined boundaries of `settings.FIRMWARE_DIR`:
    ```python
    file_path = (settings.FIRMWARE_DIR / filename).resolve()
    if not file_path.is_relative_to(settings.FIRMWARE_DIR.resolve()):
        raise HTTPException(status_code=403, detail="Access denied.")
    ```

---

## 3. End-to-End Execution Sequence

This sequence diagram illustrates a professional, secure dual-phase update cycle where the client performs an identity check, acquires a transient download token, and executes the stream download.

```
[ ESP32 Client ]                                  [ FastAPI Host ]                       [ Local Disk / Storage ]
       |                                                 |                                          |
       | 1. Query Check Request                          |                                          |
       |    Headers:                                     |                                          |
       |      X-Device-API-Key: token                    |                                          |
       |      x-ESP32-version: 1.0.0                     |                                          |
       |      x-ESP32-hardware: S3-WROOM                 |                                          |
       |------------------------------------------------>|                                          |
       |                                                 | 2. Check update against manifest.json    |
       |                                                 |----------------------------------------->|
       |                                                 |<-----------------------------------------|
       |                                                 |                                          |
       |                                                 | 3. If update available, generate:        |
       |                                                 |    - Expiration: current_time + 300s     |
       |                                                 |    - Token: hmac(filename + expiration)  |
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
```

---

## 4. Key Client-Side Implementation Fixes

The following architectural and implementation fixes are verified and embedded inside the C++ framework files to ensure runtime compliance and connectivity:

### Exception Safety (`-fno-exceptions`)
Standard-library components that throw exceptions under memory limits or invalid parsing (such as `std::stoi` and nested `try/catch` scopes) were removed. 
*   **URL Parsing**: Rewritten to utilize non-throwing `std::strtol` with strict `endptr` checking to validate port values:
    ```cpp
    char* endptr = nullptr;
    long portVal = std::strtol(portStr.c_str(), &endptr, 10);
    if (endptr == portStr.c_str() || *endptr != '\0' || portVal < 0 || portVal > 65535) {
        return ERROR_INVALID_ARG;
    }
    ```
*   **Out-Of-Memory Handling**: Structural operations (such as appending HTTP event data chunks to buffers via `std::vector::insert`) execute standard system termination handlers natively if a physical heap allocation failure occurs.

### Dynamic Port & TLS Scheme Matching
In original tests, the SSL validation parameter (`clientOptions.use_tls`) was bound strictly to port `443`. To support custom secure ports (e.g. `8443`), `_parseUrl` was modified to output an `outIsHttps` boolean parameter. The client uses this boolean parameter directly to establish TLS handshakes, regardless of the port number.

### ESP-IDF Configuration Path Compliance
During initialization inside `HttpsClient::_populate_config()`, omitting the `.path` parameter triggered ESP-IDF system-level validation errors:
```text
E (1910) HTTP_CLIENT: config should have either URL or host & path
```
To guarantee structural compliance, the initialization parameters are populated with a default fallback path of `"/"`. The actual URI target is modified dynamically before transmitting requests via `sendRequest()`.
