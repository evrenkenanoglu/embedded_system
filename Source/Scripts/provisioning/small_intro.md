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
[ STEP 1: FACTORY (Once per device) ]
  • Physical ESP32 is plugged in via USB.
  • Run provision_hardware.py:
      - Burn hardware lock bits (disable physical tampering).
      - Turn on Flash Encryption (scrambles chip storage).
      - Flash Master Root Certificate (ca.crt) into secure memory.

                  │
                  ▼
[ STEP 2: CREATING AN UPDATE (Every time you write new code) ]
  • Compile new code -> firmware.bin
  • Run main.py --step sign:
      - Hashes the binary (creates a unique digital fingerprint).
      - Signs it with developer key -> creates target_signature.
      - Uploads firmware.bin + signature + signing.crt to the Server.

                  │
                  ▼
[ STEP 3: THE DEVICE UPDATES (In the customer's home) ]
  • Phase 1 (The Check):
      - Device asks server: "Is there an update for me?"
      - Server checks: Right hardware? Newer version? Battery/schedule OK?
      - Server replies with version info + digital signature.
  • Phase 2 (The Download):
      - Device downloads the binary in small 8 KB blocks over Wi-Fi.
      - Writes data to the inactive slot (Bank B) while Bank A keeps running.
  • Phase 3 (The Verification):
      - Device checks: Is signing.crt trusted by the Master Root CA? -> YES.
      - Device calculates hash of Bank B and checks against target_signature -> MATCH.
  • Phase 4 (The Switch):
      - Device switches boot pointer to Bank B.
      - Restarts into the new firmware.
      - Runs self-test. If healthy -> confirms update. If broken -> rolls back to Bank A.
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