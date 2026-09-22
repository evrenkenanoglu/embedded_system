### Core Architecture Pillars (Platform-Agnostic Abstraction)

| Security Layer | Hardware / Silicon Anchor | Generic Interface / Firmware Abstraction | Security Guarantee |
| :--- | :--- | :--- | :--- |
| **Boot Integrity** | Silicon Root of Trust (OTP / eFuse Signature Digest) | `IBootloader` $\rightarrow$ Secure Boot Anchor Stage | Executes only binaries signed with the authorized developer public key. |
| **Data Confidentiality** | Hardware Bus Cryptographic Engine (AES-XTS) | `IHal_FlashEncryption` (Transparent inline bus encryption) | Physical SPI/bus dumps expose only ciphertext. |
| **Silicon Lockdown** | Permanent Hardware Control Registers / Lock Bits | Physical OTP Write/Read Protection Registers | Hardware debug interfaces (JTAG/SWD) and unvalidated execution paths are permanently revoked. |
| **Anti-Rollback** | Monotonic Hardware Security Counter / eFuse | `IHal_Mem_Ota::validateImageHeader()` $\rightarrow$ `IHal_Security::readHardwareSecurityVersion()` | Rejects firmware images carrying a security counter lower than the hardware threshold. |
| **Trust Anchors** | Hardware-Encrypted Key Partition | `IHal_Mem::readData()` $\rightarrow$ `IOtaManager` (Secure Partition Scope) | Root certificates reside in authenticated, encrypted storage; zero credentials in `.rodata`. |
| **Payload Integrity** | Asymmetric Cryptographic Algorithm (ECDSA / RSA) | `IOtaService` $\rightarrow$ `ICryptoEngine::verifySignature()` | Hash calculation is strictly bounded to `targetSize`; verified against validated X.509 chain. |

---

### System Architecture & Provisioning Flow

```mermaid
flowchart TD
    subgraph FACTORY["1. Factory Station (One-Time Manufacturing)"]
        CA["Master Root CA + Secondary Rotation CA"]
        SEC_KEYS["Generate Master Keys: Bus Encryption Key + Root Signature Digest"]
        NVS_GEN["Secure Storage Generator<br/>Builds encrypted factory partition with Root CA"]
        EFUSE_BURN["Silicon Hardware Provisioner<br/>1. Program Hardware Bus Encryption Key<br/>2. Program Bootloader Verification Digest<br/>3. Read-Protect Bus Encryption Key<br/>4. Write-Protect Security Register Blocks<br/>5. Revoke Debug Interfaces & Direct Boot Modes<br/>6. Flash Partition Map: Bootloader, Keystore, Trust Storage, App"]
        CA --> NVS_GEN
        SEC_KEYS --> EFUSE_BURN
        NVS_GEN --> EFUSE_BURN
    end

    subgraph CI_CD["2. Release & Code-Signing Pipeline"]
        SRC["Application Source (C++)"] --> COMPILE["Build Toolchain<br/>Compiles Signed Binary"]
        COMPILE --> HASH["Compute Target SHA-256 Digest"]
        HASH --> HSM["HSM / Release Signer<br/>Sign with Developer Private Key<br/>Output: Raw Asymmetric Signature"]
        HSM --> MANIFEST["Manifest Packager<br/>Packages size, digest, signature, and X.509 cert into manifest.json"]
    end

    subgraph DEVICE["3. Target Embedded Device (Runtime Update)"]
        CHECK["IOtaManager::checkForUpdates()<br/>Fetch update metadata via Secure Transport"]
        NVS_READ["IHal_Mem::readData()<br/>Load Primary & Backup Root CA from Secure Storage"]
        CHAIN_VAL["ICryptoEngine::verifyCertificateChain()<br/>Validate Developer Cert against Primary/Backup CA"]
        HDR_VAL["IHal_Mem_Ota::validateImageHeader()<br/>Assert: image.security_version >= IHal_Security::readHardwareSecurityVersion()"]
        STREAM["IOtaService: Stream chunks to inactive memory bank"]
        PROG_HASH["IOtaService::_calculatePartitionHash()<br/>Progressive SHA-256 strictly across targetSize bytes"]
        TRANSCODE["ICryptoEngine::normalizeSignature()<br/>Normalize signature format (IEEE P1363 to DER)"]
        SIG_VERIFY["ICryptoEngine::verifySignature()<br/>Verify digest against validated developer context"]
        BOOT_SET["IHal_Mem_Ota::setBootPartition()<br/>Switch active boot target & reboot"]
        ABORT["IHal_Mem_Ota::abort()<br/>Invalidate buffer, cancel rollback & discard"]
    end

    FACTORY -.->|Pre-provisioned Hardware| DEVICE
    CI_CD -.->|Secure Transport Delivery| CHECK

    CHECK --> NVS_READ
    NVS_READ --> CHAIN_VAL
    CHAIN_VAL -->|Invalid Trust Chain| ABORT
    CHAIN_VAL -->|Valid Trust Chain| STREAM
    STREAM --> HDR_VAL
    HDR_VAL -->|Anti-Rollback Violation| ABORT
    HDR_VAL -->|Version Allowed| PROG_HASH
    PROG_HASH --> TRANSCODE
    TRANSCODE --> SIG_VERIFY
    SIG_VERIFY -->|Signature Failure| ABORT
    SIG_VERIFY -->|Signature Match| BOOT_SET
```

---

### Cryptographic Verification & Memory Layout

```mermaid
sequenceDiagram
    autonumber
    participant Srv as OTA Gateway / Server
    participant Mgr as IOtaManager
    participant Mem as IHal_Mem (Secure Storage)
    participant Hal as IHal_Mem_Ota
    participant Svc as IOtaService
    participant Cry as ICryptoEngine

    Note over Mgr,Mem: Phase 1: Trust Anchor Initialization
    Mgr->>Mem: readData("root_ca_pem", buf)
    Mem-->>Mgr: Primary Root CA (PEM)
    Mgr->>Mem: readData("root_ca_backup_pem", buf)
    Mem-->>Mgr: Secondary Backup Root CA (PEM)

    Note over Srv,Mgr: Phase 2: Metadata Handshake
    Mgr->>Srv: Request Update Check (Current Version, Hardware Security Version, Model ID)
    Srv-->>Mgr: targetSize, signatureBytes, signingCertPem, streamEndpoint

    Note over Mgr,Cry: Phase 3: Certificate Trust Validation
    Mgr->>Cry: verifyCertificateChain(primaryCa, signingCert, backupCa)
    alt Invalid Root Trust Chain
        Cry-->>Mgr: ERROR_FAIL
        Mgr->>Hal: abort()
    else Valid Trust Chain
        Cry-->>Mgr: ERROR_SUCCESS
    end

    Note over Srv,Hal: Phase 4: Download & Anti-Rollback Pre-Check
    Svc->>Srv: Stream Payload Chunks
    Svc->>Hal: write(initialChunk)
    Hal->>Hal: validateImageHeader()
    Note over Hal: Query Silicon Register: readHardwareSecurityVersion()<br/>Assert: initialChunk.security_version >= hardware_version
    alt Rollback Attempt Detected
        Hal-->>Svc: ERROR_INVALID_STATE
        Svc->>Hal: abort()
    else Version Allowed
        Hal->>Hal: Write chunk to physical secondary execution bank
    end
    Svc->>Hal: Stream remaining chunks strictly up to targetSize

    Note over Svc,Cry: Phase 5: Progressive Integrity & Signature Verification
    Svc->>Cry: hashStart(SHA_256)
    loop Read partition blocks (offset = 0 to targetSize)
        Svc->>Hal: read(offset, buffer, chunkSize)
        Svc->>Cry: hashUpdate(buffer, chunkSize)
    end
    Svc->>Cry: hashFinish(computedDigest[32])
    Svc->>Cry: verifySignature(ALGO_ASYMMETRIC, signingCert, computedDigest, signatureBytes)
    Note over Cry: Transcode & Verify signature against public key context
    alt Verification Failure
        Cry-->>Svc: ERROR_FAIL
        Svc->>Hal: abort()
    else Signature Matches
        Cry-->>Svc: ERROR_SUCCESS
        Svc->>Hal: setBootPartition()
        Note over Hal: Update active boot pointer to secondary execution bank
    end
```