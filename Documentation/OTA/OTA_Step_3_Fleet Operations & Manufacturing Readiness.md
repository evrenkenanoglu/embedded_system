### AI Implementation Prompt

```text
You are an expert Embedded Systems Architect, Cloud Infrastructure Engineer, and Manufacturing Automation Specialist. Your objective is to implement the "Fleet Operations & Manufacturing Readiness" domain for the ESP32-S3 OTA framework and its accompanying server control plane.

### SYSTEM CONTEXT & ARCHITECTURE
- Architecture: 3-Tier Client Firmware (HAL -> PAL -> App) paired with a high-throughput, asynchronous Server Control Plane (FastAPI, Python 3.11+, PostgreSQL/SQLite, Redis).
- Hardware Target: ESP32-S3 with external Octal PSRAM (SPIRAM), eFuse security blocks, and hardware cryptographic accelerators.
- Client Constraints: C++17/C++20, -fno-exceptions, strict RAII, deterministic FreeRTOS memory isolation (internal SRAM vs. external PSRAM).
- Server Constraints: Asynchronous I/O, atomic file operations, deterministic modulo-based canary distribution, authenticated telemetry ingestion.

### CORE OBJECTIVES
1. Multi-Tier Production PKI & HSM Code-Signing Infrastructure:
   - Establish a 3-tier PKI model: Offline/Air-Gapped Root CA -> Intermediate Issuing CA (Cloud KMS / HSM) -> Short-Lived Developer Signing Certificates.
   - Implement automated signing tools that interface with PKCS#11 / AWS KMS / GCP KMS / Vault to generate ECDSA SECP256R1 signatures for firmware builds.
   - Implement certificate revocation checks (CRL / OCSP stapling) during Phase 1 check queries.
2. PSRAM Memory Allocation for Differential Updates:
   - Configure ESP-IDF external PSRAM (SPIRAM) in 8-line (Octal) mode with custom heap capabilities.
   - Modify mem_ota and esp_delta_ota integration to enforce that all delta decompression scratch buffers, patch stream buffers, and sliding dictionary tables allocate strictly from external PSRAM (MALLOC_CAP_SPIRAM) to keep internal SRAM clear for Wi-Fi/Bluetooth stacks.
3. Fleet Rollout Engine & Channel Orchestration:
   - Implement multi-channel segregation (production, beta, canary, development) on the server control plane.
   - Implement deterministic, stateless canary rollout expansion (e.g., 1% -> 5% -> 25% -> 100%) calculated via MD5 hash buckets: MD5(device_id + ":" + version) % 100 < canary_percentage.
   - Support dynamic maintenance windows and regional jitter scheduling to avoid synchronized backend traffic spikes.
4. Factory Provisioning Automation Tooling:
   - Develop an automated factory-line provisioning suite (Python CLI / GUI) interfacing with espefuse.py, espsecure.py, and nvs_partition_gen.py.
   - Automate burning of hardware Secure Boot V2 digests, Flash Encryption keys, monotonic eFuse HSVN minimums, and unique per-device mTLS certificates into encrypted NVS.
   - Generate cryptographically signed factory audit manifests mapping Device Serial, MAC Address, Hardware Revision, and Silicon Unique ID.
5. Fleet Observability & Automated Closed-Loop Rollbacks:
   - Enhance /api/v1/ota/status to ingest structured JSON telemetry (success/failure, error codes, download duration, RSSI, battery voltage).
   - Implement sliding-window failure rate aggregators on the server. If a release version breaches configured failure thresholds (e.g., >= 5% failures over minimum 20 reports), automatically mark the release as soft-rolled-back, atomically update manifest.json, revert channel pointers, and trigger alert webhooks.

### DELIVERABLES REQUIRED
- C++ PSRAM memory mapping configurations and driver patches for mem_ota.
- Complete Python manufacturing provisioning suite (factory_provisioner.py) with full command-line arguments and logging.
- Production PKI generation, signing, and KMS integration scripts (sign_release.py).
- Enhanced FastAPI server control plane endpoints (/api/v1/ota/check, /api/v1/ota/status) with atomic manifest locking and rollback evaluation.
- Integration tests verifying memory allocation boundaries, KMS signature validity, canary bucket partitioning, and automatic soft-rollback triggers.
```

---

### Fleet Operations & Manufacturing Readiness: Implementation Checklist

#### 1. Multi-Tier Production PKI & HSM Key Management
- [ ] **PKI Hierarchy Architecture:**
  - [ ] Generate Offline Root CA (4096-bit RSA or ECC SECP384R1) stored in an air-gapped environment or Hardware Security Module (HSM).
  - [ ] Provision Intermediate Issuing CA managed by a secure Key Management Service (AWS KMS, Google Cloud KMS, or HashiCorp Vault).
  - [ ] Implement short-lived Developer Signing Certificates (`signing.crt`) with maximum 30–90 day validity periods.
- [ ] **Automated Code-Signing Pipeline:**
  - [ ] Create `sign_release.py` to hash compiled binaries and invoke remote KMS/HSM signing APIs via PKCS#11.
  - [ ] Implement manifest packager bundling metadata: `target_size`, `target_signature`, `signing_cert`, `isDelta`, `target_hsvn`, and `min_hardware_rev`.
- [ ] **Certificate Revocation & Expiration Engine:**
  - [ ] Implement Certificate Revocation List (CRL) or serial blocklist generation on the server.
  - [ ] Validate certificate expiration (`notBefore` / `notAfter`) and serial revocation on the server prior to returning manifest data in `/api/v1/ota/check`.

---

#### 2. Memory Isolation & External PSRAM (SPIRAM) Hardening
- [ ] **Octal PSRAM Configuration (`sdkconfig`):**
  - [ ] Enable `CONFIG_SPIRAM=y`.
  - [ ] Configure `CONFIG_SPIRAM_MODE_OCT=y` and `CONFIG_SPIRAM_SPEED_80M=y` for ESP32-S3.
  - [ ] Set `CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=4096` to reserve small allocations for internal SRAM.
  - [ ] Enable `CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY=y` for non-ISR task stacks.
- [ ] **Delta Decompressor PSRAM Allocation (`mem_ota.cpp`):**
  - [ ] Wrap `esp_delta_ota` allocations to strictly use `heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)`.
  - [ ] Implement dynamic fallback: if PSRAM is unavailable, abort delta decompression and trigger full binary update fallback.
  - [ ] Audit internal SRAM high-water mark during active decompression to verify zero impact on network buffers.

---

#### 3. Fleet Orchestration & Channel Management
- [ ] **Channel Segregation:**
  - [ ] Configure isolated channel tracks on the control plane (`production`, `beta`, `testing`, `canary`, `development`).
  - [ ] Validate client-reported `x-ESP32-channel` headers against manifest database release streams.
- [ ] **Stateless Canary Rollout Algorithm:**
  - [ ] Implement deterministic device cohort bucketing:
    ```python
    device_bucket = int(hashlib.md5(f"{device_id}:{target_version}".encode()).hexdigest(), 16) % 100
    is_included = device_bucket < canary_percentage
    ```
  - [ ] Build deployment pipeline commands to adjust `canary_percentage` incrementally (e.g., 1% $\rightarrow$ 5% $\rightarrow$ 25% $\rightarrow$ 100%).
- [ ] **Traffic Jitter & Scheduling Control:**
  - [ ] Define backend configuration parameters for `baseCheckIntervalSec` and `jitterRangeSec`.
  - [ ] Implement server-side rate-limiting on `/api/v1/ota/check` and binary download routes to prevent CDN overload.

---

#### 4. Mass Production & Factory Provisioning Automation
- [ ] **Factory Provisioning CLI Tool (`factory_provision.py`):**
  - [ ] **Step 1:** Generate unique per-device identity (Device ID, UUID, random mTLS client key).
  - [ ] **Step 2:** Generate and burn Flash Encryption key into eFuse `BLOCK_KEY0` / `BLOCK_KEY1`.
  - [ ] **Step 3:** Generate Secure Boot V2 key digest and burn into eFuse `BLOCK_KEY2` / `BLOCK_KEY3`.
  - [ ] **Step 4:** Burn Initial Monotonic Anti-Rollback Version (`SECURE_VERSION = 1`).
  - [ ] **Step 5:** Burn hardware security lock bits (`DIS_PAD_JTAG`, `DIS_DOWNLOAD_MODE`, `WR_DIS`).
  - [ ] **Step 6:** Generate and flash encrypted NVS binary (`nvs_encrypted.bin`) containing the Root CA certificate, device credentials, and calibration values.
  - [ ] **Step 7:** Flash initial production bootloader, partition table, and factory firmware image.
- [ ] **Manufacturing Audit Logging:**
  - [ ] Log serial number, factory station ID, MAC address, eFuse SHA-256 digests, and timestamp to a secure manufacturing database.
  - [ ] Execute post-flash verification test validating that Secure Boot and Flash Encryption successfully enable on initial boot.

---

#### 5. Telemetry Ingestion, Fleet Observability & Automated Rollback
- [ ] **Telemetry Ingestion Endpoint (`POST /api/v1/ota/status`):**
  - [ ] Ingest structured payload: `device_id`, `previous_version`, `target_version`, `status`, `error_code`, `download_time_ms`, `rssi`, `battery_pct`.
  - [ ] Sanitize input strings to prevent directory traversal and SQL/NoSQL injection.
  - [ ] Persist telemetry events into a timeseries or relational data store with indexing on `target_version` and `status`.
- [ ] **Automated Closed-Loop Rollback Engine:**
  - [ ] Implement real-time metric evaluator checking versions against configured limits:
    - Minimum sample threshold: `MIN_REPORTS_FOR_EVALUATION` (e.g., $\ge 20$ devices).
    - Maximum failure threshold: `MAX_FAILURE_RATE_PERCENT` (e.g., $\ge 5.0\%$).
  - [ ] On failure threshold breach:
    - [ ] Mark target version status as `soft-rolled-back` in `manifest.json`.
    - [ ] Atomically point active channel `latest_version` to the last known stable release using atomic rename (`manifest.json.tmp` $\rightarrow$ `manifest.json`).
    - [ ] Invalidate active presigned download tokens for the failed version.
    - [ ] Dispatch critical alert webhooks to monitoring platforms (Slack, PagerDuty, Datadog).