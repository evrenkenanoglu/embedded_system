### AI Implementation Prompt

```text
You are an expert Embedded Systems Architect, Cloud Infrastructure Engineer, and Manufacturing Automation Specialist. Your objective is to implement the "Fleet Operations & Manufacturing Readiness" domain (Phase 3) for the firmware update framework and its server control plane.

### SYSTEM CONTEXT & ARCHITECTURE
- Architecture: 3-Tier Layered Client Firmware (HAL -> PAL -> App) paired with a high-throughput, asynchronous Server Control Plane (FastAPI, Python 3.11+, PyYAML, Jinja2, Requests).
- Hardware Target: ESP32-S3 with or without external PSRAM (SPIRAM), silicon eFuse blocks, transparent hardware Flash Encryption, and Secure Boot V2.
- Core Invariants: Single Source of Truth (SSoT via configs/config_project.yaml), zero duplicate scripts (extend existing provision_hardware.py and hsm_sign_digest.py), strict backward compatibility with existing Invoke tasks (es.crypto.*, es.ota.*).
- Client Constraints: C++17/C++20, -fno-exceptions, strict RAII, deterministic FreeRTOS memory isolation (internal SRAM vs. external PSRAM fallback).
- Server Constraints: Asynchronous I/O, atomic file operations, deterministic modulo-based canary distribution, authenticated telemetry ingestion.

### CORE OBJECTIVES
1. Multi-Tier Production PKI & Remote KMS/HSM Detached Signing:
   - Establish a 3-tier PKI model: Offline Root CA -> Intermediate Issuing CA (Cloud KMS / HSM) -> Short-Lived Developer Signing Certificates.
   - Extend Source/Scripts/provisioning/signing/hsm_sign_digest.py to interface with Cloud KMS (AWS KMS / HashiCorp Vault) and PKCS#11 APIs using the existing detached signing architecture (--stage digest, --stage assemble).
   - Enforce Certificate Revocation List (CRL) and serial number blocklist checks on the server control plane during /api/v1/ota/check queries.
2. Dynamic Memory Capability Allocation (Delta Decompression):
   - Modify mem_ota.cpp and delta decompressor scratchpad allocations to dynamically query heap capabilities: allocate from external PSRAM (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) if fitted, and automatically fall back to internal heap (MALLOC_CAP_INTERNAL) if PSRAM is absent or disabled.
3. Fleet Rollout Engine & Channel Orchestration:
   - Implement multi-channel segregation (stable, beta, testing, development) across client request headers and server manifest streams.
   - Enhance the stateless canary rollout algorithm (MD5(device_id:version) % 100 < canary_percentage) with incremental stage transitions (1% -> 5% -> 25% -> 100%).
   - Enforce traffic jitter windows and server-side rate limiting to avoid backend traffic spikes.
4. Mass Manufacturing Factory Fixture Automation:
   - Enhance Source/Scripts/provisioning/factory/provision_hardware.py with an automated assembly-line continuous mode (--continuous): auto-detect serial connection, query MAC, burn Flash Encryption key (BLOCK_KEY0) and Secure Boot V2 digest (BLOCK_KEY1), apply silicon read/write locks, burn monotonic anti-rollback minimums (SECURE_VERSION), and flash encrypted partitions.
   - Extend Source/Scripts/provisioning/factory/audit_logger.py to aggregate manufacturing audit records into a consolidated CSV ledger (build/audit_logs/manufacturing_summary.csv).
5. Fleet Observability & Closed-Loop Rollbacks:
   - Upgrade /api/v1/ota/status to ingest structured JSON telemetry (status, error_code, target_version, previous_version, device_id).
   - Implement a sliding-window failure rate aggregator on the server (evaluating only events within trailing 3600s). If failures exceed configured thresholds (e.g., >= 10% over >= 5 reports), atomically transition the version to soft-rolled-back in manifest.json, revert active channel pointers, and dispatch alert webhooks (Slack/Teams/PagerDuty).

### DELIVERABLES REQUIRED
- Driver updates in mem_ota.cpp implementing dynamic PSRAM/internal SRAM heap capability fallbacks.
- Enhanced provision_hardware.py supporting continuous fixture batch programming (--continuous) and audit log consolidation.
- Cloud KMS / Vault detached signing integration in hsm_sign_digest.py.
- Enhanced FastAPI server control plane endpoints (/api/v1/ota/check, /api/v1/ota/status) with sliding-window telemetry aggregation, CRL validation, and incident webhook dispatching.
- Automated fleet integration test suite (CI_CD/tests/hil/test_fleet_operations.py).
```

---

### Fleet Operations & Manufacturing Readiness: Implementation Checklist

#### 1. Multi-Tier Production PKI & Remote KMS/HSM Integration
- [ ] **PKI Hierarchy Configuration:**
  - [ ] Support Offline/Air-Gapped Root CA (`certs/ca.crt`) and Cloud KMS / Vault Intermediate CA structures.
  - [ ] Implement short-lived Developer Signing Certificates (`certs/signing.crt`) with configurable validity boundaries.
- [ ] **Remote KMS Detached Signing Adapter (`hsm_sign_digest.py`):**
  - [ ] Add `--kms-provider [none|aws-kms|vault]` parameter to `hsm_sign_digest.py`.
  - [ ] When `--kms-provider aws-kms`: Sign the exported 32-byte SHA-256 digest via AWS KMS `Sign` API and convert resulting DER signature to raw 64-byte IEEE P1363 ($R \parallel S$).
  - [ ] When `--kms-provider vault`: Dispatch digest to HashiCorp Vault transit secrets engine.
  - [ ] Retain local private key signing (`--stage all`) for development and local testing.
- [ ] **Certificate Revocation List (CRL) Engine (`ota-server/src/core/security.py`):**
  - [ ] Add CRL file (`certs/revoked.crl`) and serial blocklist support in `configs/config_ota_server.yaml`.
  - [ ] During `GET /api/v1/ota/check`, extract developer certificate serial number:
    - [ ] Reject update with `HTTP 403 Forbidden` if certificate serial exists in CRL/blocklist.
    - [ ] Reject update if current time exceeds certificate `notBefore` / `notAfter` boundaries.

---

#### 2. Memory Isolation & External PSRAM Fallback Hardening
- [ ] **Dynamic Heap Capability Selection (`mem_ota.cpp`):**
  - [ ] Inspect available heap capabilities using `esp_heap_caps_get_free_size(MALLOC_CAP_SPIRAM)`.
  - [ ] If PSRAM is detected and configured: Allocate delta decompression scratch buffers and dictionary tables with `MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT` to preserve internal SRAM.
  - [ ] If PSRAM is absent: Fall back dynamically to `MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT`.
  - [ ] If total free heap is insufficient for delta patch dictionary (< 64 KB), abort delta mode with `ERROR_OUT_OF_MEMORY` to trigger full binary download fallback.

---

#### 3. Fleet Orchestration & Channel Management
- [ ] **Multi-Channel Management:**
  - [ ] Enforce isolated deployment channels (`stable`, `beta`, `testing`, `development`) in `config_ota_server.yaml`.
  - [ ] Validate client `x-ESP32-channel` headers against manifest database release streams.
- [ ] **Stateless Canary Cohort Evaluation (`ota-server/src/api/ota.py`):**
  - [ ] Verify deterministic device allocation:
    ```python
    device_bucket = int(hashlib.md5(f"{device_id}:{target_version}".encode()).hexdigest(), 16) % 100
    is_eligible = device_bucket < canary_percentage
    ```
  - [ ] Build administrative endpoint / dashboard controls to step canary rollouts (e.g., 1% $\rightarrow$ 5% $\rightarrow$ 25% $\rightarrow$ 100%).
- [ ] **Traffic Jitter & Scheduling Constraints:**
  - [ ] Configure `baseCheckIntervalSec` and `jitterRangeSec` in Master SSoT (`config_project.yaml`).
  - [ ] Enforce rate-limiting on `/api/v1/ota/check` and firmware binary download routes to prevent traffic spikes.

---

#### 4. Mass Production & Factory Provisioning Automation
- [ ] **Continuous Fixture Mode (`provision_hardware.py`):**
  - [ ] Add `--continuous` flag to `provision_hardware.py`:
    - [ ] Poll serial ports to detect device connection.
    - [ ] Query target MAC address via `esptool.py`.
    - [ ] Assert silicon is unprovisioned before executing write operations.
    - [ ] Burn Flash Encryption key into eFuse `BLOCK_KEY0` with permanent read/write protection.
    - [ ] Burn Secure Boot V2 digest into eFuse `BLOCK_KEY1` with permanent write protection.
    - [ ] Burn monotonic anti-rollback version (`SECURE_VERSION = 1`).
    - [ ] Burn silicon security lock bits (`DIS_PAD_JTAG`, `DIS_USB_JTAG`, `DIS_DIRECT_BOOT`, `DIS_DOWNLOAD_ICACHE`, `DIS_DOWNLOAD_DCACHE`).
    - [ ] Flash pre-encrypted NVS binary (`fctry`) containing Root CA trust anchors and device identity.
    - [ ] Flash bootloader (`0x0`), partition table (`0x10000`), `nvs_keys` (`0x34000`), and factory app (`0x50000`).
    - [ ] Display visual PASS/FAIL status and wait for board disconnection before priming next cycle.
- [ ] **Manufacturing Summary Ledger (`audit_logger.py`):**
  - [ ] Append every provisioned unit record to `build/audit_logs/manufacturing_summary.csv`.
  - [ ] Record timestamp, station ID, MAC address, device ID, burned key hashes, and final status.

---

#### 5. Telemetry Ingestion & Closed-Loop Emergency Rollback
- [ ] **Structured Telemetry Ingestion (`ota-server/src/api/ota.py`):**
  - [ ] Ingest client reports on `POST /api/v1/ota/status`: `device_id`, `previous_version`, `target_version`, `status` (`success`, `failure`, `rollback`), and `error_code`.
  - [ ] Store telemetry payloads in persistent JSON logs with sanitized file paths (`device_<MAC>.json`).
- [ ] **Sliding-Window Failure Aggregator:**
  - [ ] Implement sliding-window filter in `evaluate_auto_rollback()` (trailing 3600 seconds).
  - [ ] Prevent historical errors from older firmware builds skewing failure rates of newly deployed releases.
- [ ] **Automated Emergency Rollback Execution:**
  - [ ] When failure rate $\ge 10.0\%$ over $\ge 5$ device reports within the active window:
    - [ ] Transition target release status to `soft-rolled-back` in `manifest.json`.
    - [ ] Atomically revert active channel `latest_version` pointer to the last known stable release using atomic rename (`manifest.json.tmp` $\rightarrow$ `manifest.json`).
    - [ ] Invalidate active presigned HMAC download tokens for the compromised release.
- [ ] **Incident Webhook Dispatcher (`ota-server/src/core/notifications.py`):**
  - [ ] Post structured JSON alert notifications to configured webhook URLs (Slack, Teams, PagerDuty) detailing failed version, failure percentage, and sample counts.

---

#### 6. End-to-End Fleet Integration & Verification Suite
- [ ] **Automated Test Suite (`CI_CD/tests/hil/test_fleet_operations.py`):**
  - [ ] **Test 1:** Verify canary bucket distribution across 1000 synthetic device IDs at 10%, 25%, and 50% rollout thresholds.
  - [ ] **Test 2:** Simulate client rollback telemetry and assert that the server triggers atomic manifest soft-rollback.
  - [ ] **Test 3:** Verify that revoked certificate serial numbers return `HTTP 403 Forbidden` on `/api/v1/ota/check`.
  - [ ] **Test 4:** Assert that continuous fixture mode correctly generates consolidated audit CSV entries.
