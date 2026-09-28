### Core Architecture Pillars (Platform-Agnostic Abstraction)

| Operational Layer | Cloud / Manufacturing Infrastructure | Generic Interface / Platform Abstraction | Operational Guarantee |
| :--- | :--- | :--- | :--- |
| **Air-Gapped / Cloud KMS Signing** | Remote Hardware Security Module (AWS KMS / HashiCorp Vault / PKCS#11) | `hsm_sign_digest.py` (`--stage digest` $\rightarrow$ KMS $\rightarrow$ `--stage assemble`) | Developer private keys never touch developer workstations or CI runners; raw 64-byte IEEE P1363 signatures generated remotely [19]. |
| **Trust Revocation & Gating** | Certificate Revocation List (CRL) & Serial Blocklist | `verify_certificate_status()` $\rightarrow$ `GET /api/v1/ota/check` | Revoked or expired developer signing certificates are immediately barred from distributing updates across the fleet. |
| **Stateless Canary Cohorts** | Deterministic Modulo Partitioning | `is_in_canary_group(device_id, version, percentage)` | Staggered deployment rings (1% $\rightarrow$ 5% $\rightarrow$ 25% $\rightarrow$ 100%) distribute firmware deterministically without maintaining database state per device. |
| **Closed-Loop Fleet Protection** | Trailing Sliding-Window Telemetry Evaluator (3600s) | `evaluate_auto_rollback()` $\rightarrow$ `_soft_rollback_version_in_manifest()` | Automatically deactivates failing builds and rolls back channel pointers if failure rate $\ge 10\%$ over $\ge 5$ devices. |
| **Incident Observability** | Outbound Webhook Dispatcher | `dispatch_rollback_alert()` $\rightarrow$ Alert Webhook (Slack / Teams / PagerDuty) | Zero-delay notification to on-call engineering teams detailing failure rates, revoked versions, and fallback targets upon an automated rollback. |
| **Continuous Assembly Fixture** | Automated Flashing Fixture & Serial Device Poller | `provision_hardware.py --continuous` $\rightarrow$ `AuditLogger` | Assembly-line fixture detects device insertion, executes silicon locks, flashes encrypted images, and outputs a consolidated CSV summary ledger [11, 13]. |

---

### System Architecture & Manufacturing Flow

```mermaid
flowchart TD
    subgraph MANUFACTURING["1. Mass Manufacturing Station (Assembly Line)"]
        FIXTURE["provision_hardware.py --continuous<br/>Polling USB Serial Interface"]
        BOARD["Operator Connects New Target Device"] --> FIXTURE
        FIXTURE --> PROVISION["Execute Silicon Hardening:<br/>1. Program & Lock BLOCK_KEY0 / BLOCK_KEY1<br/>2. Burn SECURE_VERSION = 1 & JTAG Lock Bits<br/>3. Flash Encrypted fctry NVS & App Layout"]
        PROVISION --> AUDIT["audit_logger.py<br/>1. Write audit_MAC.json<br/>2. Append row to manufacturing_summary.csv"]
        AUDIT --> PROMPT["Display PASS/FAIL Indicator<br/>Wait for Device Disconnect"]
        PROMPT --> BOARD
    end

    subgraph CLOUD_KMS["2. Cloud KMS & Production Code-Signing"]
        BUILD["Build Pipeline (CI/CD)"] --> DIGEST["hsm_sign_digest.py --stage digest<br/>Export SHA-256 Digest Binary"]
        DIGEST --> KMS{"Remote Signer<br/>(AWS KMS / Vault)"}
        KMS --> DER_SIG["Return ASN.1 DER Signature"]
        DER_SIG --> ASSEMBLE["hsm_sign_digest.py --stage assemble<br/>Transcode to 64B IEEE P1363 (R || S)<br/>Inject into manifest.json"]
    end

    subgraph CONTROL_PLANE["3. OTA Server Control Plane"]
        CHECK_API["GET /api/v1/ota/check"]
        STATUS_API["POST /api/v1/ota/status"]
        CRL_CHECK{"verify_certificate_status()<br/>Serial in CRL or Blocklist?"}
        CANARY_EVAL{"is_in_canary_group()<br/>MD5(device_id:version) % 100 < %"}
        SLIDING_EVAL["evaluate_auto_rollback()<br/>Trailing 3600s Failure Rate >= 10.0%?"]
        ATOMIC_ROLLBACK["_soft_rollback_version_in_manifest()<br/>1. Set status: 'soft-rolled-back'<br/>2. Revert channel latest_version<br/>3. Invalidate presigned tokens"]
        WEBHOOK["dispatch_rollback_alert()<br/>Dispatch JSON alert to PagerDuty/Slack"]

        CHECK_API --> CRL_CHECK
        CRL_CHECK -->|Revoked / Expired| REJECT["Return HTTP 403 / No Update"]
        CRL_CHECK -->|Valid| CANARY_EVAL
        CANARY_EVAL -->|Included| SERVE["Issue Presigned Download Token"]
        CANARY_EVAL -->|Excluded| BYPASS["Return update_available: false"]

        STATUS_API --> SLIDING_EVAL
        SLIDING_EVAL -->|Threshold Breached| ATOMIC_ROLLBACK
        ATOMIC_ROLLBACK --> WEBHOOK
    end

    ASSEMBLE -.->|Deploy Release| CONTROL_PLANE
```

---

### Fleet Operations, Canary Rollout & Closed-Loop Rollback Sequence

```mermaid
sequenceDiagram
    autonumber
    participant DevA as Device A (Canary 10%)
    participant DevB as Device B (Non-Canary)
    participant Srv as OTA Server Gateway
    participant DB as manifest.json
    participant Hook as Incident Webhook (Slack/PagerDuty)

    Note over DevA,Srv: Scenario 1: Canary Gated Query
    DevA->>Srv: GET /api/v1/ota/check (v1.0.0, Channel: stable)
    Srv->>Srv: verify_certificate_status(signing_cert) -> Valid
    Srv->>Srv: is_in_canary_group("DEV-A", "1.1.0", 10%) -> TRUE
    Srv-->>DevA: HTTP 200 (update_available: true, target: v1.1.0, token)

    DevB->>Srv: GET /api/v1/ota/check (v1.0.0, Channel: stable)
    Srv->>Srv: is_in_canary_group("DEV-B", "1.1.0", 10%) -> FALSE
    Srv-->>DevB: HTTP 200 (update_available: false, "Bypassed by canary ring")

    Note over DevA,Srv: Scenario 2: Trial Boot Crash & Telemetry Ingestion
    DevA->>DevA: Downloads v1.1.0, reboots, executes self-tests
    Note over DevA: Diagnostic Self-Test Fails (e.g. storage error)<br/>Saves OtaRollbackDiagnostic_t & rolls back to v1.0.0
    DevA->>Srv: POST /api/v1/ota/status (status: "failure", failed_version: "1.1.0", err: 0x104)
    Srv-->>DevA: HTTP 200 OK ("recorded")

    Note over Srv,Hook: Scenario 3: Closed-Loop Automated Fleet Rollback
    loop Multiple Canary Device Reports
        Note over Srv: 5 failures reported within trailing 3600 seconds (Failure Rate >= 10.0%)
        Srv->>Srv: evaluate_auto_rollback("1.1.0") -> Breach Confirmed!
    end
    Srv->>DB: Atomic replace: manifest.json (v1.1.0 status = "soft-rolled-back", latest = "1.0.0")
    Srv->>Hook: POST webhook_url (AUTOMATIC_EMERGENCY_ROLLBACK alert payload)
    Hook-->>Srv: HTTP 200 OK

    Note over DevA,Srv: Scenario 4: Subsequent Fleet Inquiries Post-Rollback
    DevA->>Srv: GET /api/v1/ota/check (v1.0.0, Channel: stable)
    Srv->>DB: Read manifest.json (v1.1.0 is soft-rolled-back, active is v1.0.0)
    Srv-->>DevA: HTTP 200 (update_available: false, "Firmware is already up-to-date")
```
