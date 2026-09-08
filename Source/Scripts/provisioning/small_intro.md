### The System in Plain Language

The goal of this system is to **remotely update the software on an ESP32 smart plug safely over the internet**. 

It prevents two major risks:
1. **Hackers:** An attacker cannot install fake or modified code on the device.
2. **Bricking:** If power cuts out, the Wi-Fi drops, or the new code has a bug, the plug never breaks; it stays on or rolls back to the working version.

---

### The 3 Main Roles

```
┌────────────────────────────────┐     ┌────────────────────────────────┐     ┌────────────────────────────────┐
│      1. FACTORY STATION        │     │        2. OTA SERVER           │     │       3. ESP32 DEVICE          │
│        (Manufacturing)         │     │         (The Cloud)            │     │         (Smart Plug)           │
├────────────────────────────────┤     ├────────────────────────────────┤     ├────────────────────────────────┤
│ • Locks chip hardware (eFuses) │     │ • Stores compiled binary files │     │ • Checks for new versions      │
│ • Turns on Flash Encryption    │     │ • Holds manifest.json catalog  │     │ • Downloads in small chunks    │
│ • Stores Master Root CA in NVS │     │ • Grants temporary download    │     │ • Checks digital signatures    │
│ • Flashes initial software     │     │   links (tokens)               │     │ • Writes to secondary bank (B) │
└────────────────────────────────┘     └────────────────────────────────┘     └────────────────────────────────┘
```

---

### Simple Flow Diagram

```
[ STEP 0: PKI AUTHORITY (Run once per product line / CA cycle) ]
  • Run Tools/PKI/generate_pki.py:
      - Creates Master Root CA (ca.key, ca.crt).
      - Creates Developer Signing pair (signing.key, signing.crt).
      - Creates Server HTTPS pair (server.key, server.crt).
      - Creates Hardware Silicon keys (flash_encryption_key.bin, secure_boot_digest.bin).

                  │
                  ▼
[ STEP 1: FACTORY (Once per manufactured device) ]
  • Physical ESP32 is connected via USB.
  • Run runner.py --provision:
      - Burns Secure Boot V2 digest & Flash Encryption key to eFuses.
      - Burns monotonic anti-rollback version (SECURE_VERSION) and hardware lock bits.
      - Generates AES-XTS encrypted NVS partition (fctry) containing ca.crt and Device ID.
      - Flashes bootloader, partition table, factory firmware, nvs_keys, and fctry.

                  │
                  ▼
[ STEP 2: CREATING AN UPDATE (Every time you release new code) ]
  • Run idf.py build -> produces compiled application binary.
  • Run runner.py --sign:
      - Hashes the binary (SHA-256).
      - Signs digest with signing.key -> creates target_signature.
      - Packages target_signature, target_size, and signing.crt into manifest.json.
      - Uploads/moves binary and manifest.json to the OTA server.

                  │
                  ▼
[ STEP 3: THE DEVICE UPDATES (In the field) ]
  • Phase 1 (The Check):
      - Device queries GET /api/v1/ota/check with API key, HSVN, and version headers.
      - Server checks: Hardware match? Newer version? HSVN anti-rollback valid? Canary rollout group?
      - Server returns metadata (target_size, target_signature, signing_cert) + transient download token.
  • Phase 2 (The Download & Flash):
      - Two-stage safety check: Battery SoC >= 80%? Scheduled hour window met?
      - Device streams binary in 8 KB chunks over TLS.
      - Writes to inactive partition slot (Bank B) while Bank A continues running.
      - In-place delta decompression via esp_delta_ota if delta patch was served.
  • Phase 3 (Cryptographic Integrity Verification):
      - Device verifies signing.crt chain of trust against ca.crt stored in fctry NVS.
      - Device calculates progressive SHA-256 hash of Bank B strictly up to target_size.
      - Verifies calculated hash against target_signature using public key from signing.crt.
  • Phase 4 (The Switch & Self-Healing):
      - On signature match: Sets Bank B as active boot partition.
      - Restarts into new firmware.
      - Executes provisional self-tests (peripherals, NVS, network link).
      - If self-tests pass: Commits partition permanently (cancels rollback).
      - If app panics or self-tests fail: Hardware watchdog/bootloader rolls back to Bank A.
      - Reports outcome to POST /api/v1/ota/status.
```

---

### What Each Tool Does

| Tool / Script | Simple Purpose |
| :--- | :--- |
| **`partitions.csv`** | The **map** of the chip flash memory (splits memory into Bank A, Bank B, and encrypted settings). |
| **`config.yaml`** | The **control panel** where you set your project paths, version numbers, and hardware keys. |
| **`partition_nvs_generator.py`** | Creates an encrypted storage file (`nvs_encrypted.bin`) containing the Master Root Certificate and Device ID. |
| **`hsm_sign_digest.py`** | Takes your compiled binary and **stamps it with a digital signature** so the device can prove it came from you. |
| **`provision_hardware.py`** | The **factory tool** that burns the hardware security fuses on the chip and flashes the first factory image. |
| **`main.py`** | The **master command** that runs NVS generation, code signing, and hardware flashing in one command. |

---

### The 3 Safety Guarantees

1. **Dual-Bank (A/B) Memory:** The device never overwrites the software it is currently running. If the Wi-Fi cuts out during a 99% download, Bank A is completely untouched and the plug keeps working.
2. **Silicon Trust Anchor:** The Master Root Certificate is embedded inside encrypted storage. The device will reject any firmware update that isn't signed by an authorized developer key.
3. **Automated Rollback:** If the new software crashes on boot, the hardware watchdog timer automatically reboots the plug back into the previous working software slot.