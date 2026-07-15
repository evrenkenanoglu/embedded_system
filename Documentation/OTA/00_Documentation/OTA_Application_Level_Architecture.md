# ADVANCED OTA APPLICATION-LEVEL ARCHITECTURE: ADAPTIVE PULL-ON-TRIGGER

---

## 1. Transport & Trigger Matrix

| Network Profile            | Protocol                    | Trigger Mechanism                                         | Efficiency Vector                                                        | Disadvantages & Tradeoffs                                                                                                |
| :------------------------- | :--------------------------- | :---------------------------------------------------------- | :--------------------------------------------------------------------------- | :--------------------------------------------------------------------------------------------------------------------------- |
| **Cellular / Mains**       | CoAP / MQTT (Clean Session) | 1-bit flag (`update_available: 1`) in telemetry ACK.      | Eliminates dedicated polling connections; piggybacks on existing uplink. | High latency for critical patches; updates are gated entirely by the telemetry frequency.                                |
| **LPWAN (LoRaWAN/NB-IoT)** | Class A Downlink            | Cloud queues downlink to match ephemeral Rx1/Rx2 windows. | Zero-power standby; device radio remains off until signaled.             | Extreme payload constraints; downloading even small binaries takes substantial time due to duty-cycle limits.            |
| **Local (Wi-Fi/Ethernet)** | mDNS / UDP Broadcast        | Local edge-node service discovery advertisement.          | Bypasses WAN egress costs; executes local-speed distribution.            | Requires local hardware infrastructure; multicast traffic is frequently blocked by enterprise network security policies. |

---

## 2. Advanced Orchestration & Fleet Operations

### Deterministic Canary Division
To prevent bad firmware releases from impacting an entire fleet simultaneously, updates are rolled out in staggered cohorts. The server evaluates target inclusion dynamically and deterministically without storing state.

* **The Formula:**
  $$\text{Target Inclusion} = \left( \text{MD5}(\text{DeviceID} + \text{":"} + \text{TargetVersion}) \pmod{100} \right) < \text{CanaryPercentage}$$
* **The Mechanics:** Using this modulo arithmetic, a device is assigned a persistent placement value $[0 \dots 99]$ for any given firmware version. If the server increases the version's `canary_percentage` from $10\%$ to $25\%$, the expansion safely and deterministically includes the original devices plus the next $15\%$ without recalculating previous groupings.

### Channel Partitioning
Devices are segmented into isolated deployment channels (e.g., `stable`, `beta`, `testing`) to run targeted pilots without configuration drift. The update discovery process isolates candidate binaries using the device's self-reported channel identifier.

### Closed-Loop Automated Rollback (Anti-Brick Guard)
The system operates a continuous telemetry feedback loop to safeguard against runtime faults or "silent bricks."

```
  [ Server serves v1.1.0 ] ────► [ Target Device Downloads & Boots ]
             ▲                                       │
             │ (Auto-Deactivates Version)            ▼
      (Failure Threshold Hit?)               [ Post Status Payload ]
             │                                 - success / failure
             │                                 - error codes
             └───────────────────────────────────────┘
```

* **The Mechanism:** 
  1. The target device attempts to apply the OTA update.
  2. Upon success or failure, the device transmits a status report back to `/api/v1/ota/status`.
  3. The server tracks these reports per version. If the failure rate equals or exceeds the defined limit (e.g., $\ge 10\%$) over a minimum sample size (e.g., $\ge 5$ devices), the server automatically downgrades the version's status from `active` to `soft-rolled-back` in `manifest.json`.
  4. Subsequent update queries automatically fall back to the last known stable version for that channel, halting further deployment to unaffected devices.

---

## 3. Core Execution Policies

### Randomized Jitter (Anti-Congestion)
When a fleet of devices is configured to check for updates on a fixed schedule, they create a massive concurrent traffic spike (the "thundering herd" problem) that can crash the hosting servers or CDNs.

* **The Formula:**
  $$\text{Next Connection Interval} = T_{\text{base}} \pm \text{RandomOffset}$$
* **The Explanation:** Adding a randomized time modifier (e.g., $\pm 30$ minutes) to the scheduling timer distributes the server request load evenly over a wider window, ensuring smooth, predictable resource utilization on the backend.
* **Disadvantages:**
  * **Unpredictable Update Completion Times:** Makes fleet-wide deployment verification difficult to track in real-time, as devices apply updates at highly variable times.
  * **Debugging Complexity:** Troubleshooting connectivity logs becomes harder because there is no predictable time window when a specific device is expected to initiate its handshake.

### Binary Delta Compression (Differential Updates)
Transmitting a complete, monolithic firmware binary over low-bandwidth or metered connections is highly inefficient and prone to mid-transfer failures.

* **The Mechanics:** Instead of sending the full image, a differential algorithm (e.g., BSPatch or detools) compares the old binary with the new binary at compile time. It generates a "delta" file containing only the specific bytes that have changed. 
* **The Explanation:** The device downloads this highly compressed delta file (frequently reducing a 4MB payload down to 100KB) and uses an on-chip patching utility to reconstruct the new firmware version directly in its flash memory.
* **Disadvantages:**
  * **Computational Overhead:** The reconstruction process is memory and CPU-intensive. Resource-constrained microcontrollers may lack the RAM required to run the decompression algorithm.
  * **Dependency Chains:** Delta files are strictly version-dependent. The release pipeline must generate, store, and manage a matrix of delta files for every historical firmware version currently active in the field.

### Two-Stage Conditional Handshake
Downloading a large firmware payload is a high-risk operation that can drain the remaining battery or corrupt the system if interrupted by power failure.

```
[ Phase 1: Metadata Check ] ──(Update Found)──> [ Phase 2: Safety Gate ]
                                                       │
                                            (Battery > 80%? RSSI Good?)
                                                       │
                                                       ▼
                                            [ Execute Heavy Download ]
```

* **The Mechanics:** 
  * **Stage 1 (Metadata):** The device performs a lightweight check to discover if an update exists and caches the target version metadata.
  * **Stage 2 (Condition Verification):** The actual download of the heavy binary is deferred until specific safety guards are met.
* **The Explanation:** The device delays the download until a low-risk window occurs—such as when the device is connected to external power, when the battery state of charge (SoC) is above 80%, during off-peak system hours (e.g., 02:00 AM local time), or when the RF signal quality (RSSI) is stable enough to prevent packet loss.
* **Disadvantages:**
  * **State Machine Complexity:** The firmware bootloader and application must maintain and persist a "pending update" state across sleep cycles, reboots, and network disconnects.
  * **Perpetual Outdate Risks:** If a device's battery degrades and can never reach the required charging threshold, or if it is installed in a permanently low-signal environment, the device will remain unpatched indefinitely without throwing an explicit network error.

---

## 4. Storage Partitioning (A/B Dual-Bank Layout)
To ensure the device remains operational during an update process, the internal or external flash storage must be divided into dual-boot banks.

```
┌───────────────────────────────────────────────────────────┐
│                     Internal Flash                        │
├─────────────┬─────────────┬───────────────┬───────────────┤
│ Bootloader  │ Primary (A) │ Secondary (B) │  User Data    │
│ (ReadOnly)  │ Active App  │  OTA Target   │  (Persistent) │
└─────────────┴─────────────┴───────────────┴───────────────┘
```

* **The Mechanics:** The bootloader resides in a write-protected partition. The application space is divided into two identical banks: Active (running the current OS/app) and Staging (receiving the incoming download). 
* **The Benefit:** If the transfer fails midway or power is cut during downloading, the Active partition remains completely untouched and functional. The download can resume or restart without interrupting device operations.
* **Disadvantages:**
  * **Double Memory Footprint:** Requires exactly double the flash space for application code, which raises hardware unit costs.
  * **Partition Alignment Constraints:** Both banks must be sized to fit the absolute maximum possible application binary size, limiting feature growth.

---

## 5. Security & Integrity Validation (Secure Boot & Anti-Downgrade)
Without cryptographic verification, an OTA system is vulnerable to malicious firmware injections and downgrade attacks.

```
  [ Compiled Binary ]
          │
          ├─► Generate SHA-256 Hash ─► Sign with Private Key (ECDSA/Ed25519)
          │                                      │
          ▼                                      ▼
[ Encrypted/Signed Firmware Package: Binary + Signature ]
                               │
                       (Network Transfer)
                               │
                               ▼
                        [ Target MCU ]
                               │
                               ├─► Read Public Key (from Secure Element/OTP)
                               ├─► Compute local SHA-256 Hash
                               ▼
                    [ Signature Valid? ] 
                     ├── YES ──► Write to Flash
                     └── NO  ──► Wipe Partition
```

* **The Mechanics:**
  * **Authenticity:** Developers sign compiled binaries with an asymmetric private key (e.g., ECDSA or Ed25519). The target device stores the corresponding public key in hardware-locked OTP (One-Time Programmable) memory or a Secure Element.
  * **Anti-Downgrade:** The bootloader or system validation agent compares the incoming firmware's Hardware Security Version Number (HSVN) against a monotonic hardware counter stored in secure eFuses.
* **The Benefit:** Prevents Man-in-the-Middle (MitM) attacks from pushing corrupted or malicious code, and blocks attackers from downgrading the firmware to an older version containing known vulnerabilities.
* **Disadvantages:**
  * **Key Management Overhead:** Loss of the private signing key completely halts the ability to release future updates to existing devices.
  * **Increased Boot Latency:** Cryptographic signature verification at boot introduces a delay before the application executes, which can affect time-sensitive startups.

---

## 6. Self-Test & Automated Rollback (Anti-Brick Guard)
An update that successfully compiles and flashes may still crash at runtime due to environmental bugs, leading to permanently "bricked" devices.

```
[ Bootloader ] ─► Boot Bank B (New App) ─► Start Rollback Timer (Watchdog)
                                                  │
                                            [ Execute App ]
                                                  │
                                      ┌───────────┴───────────┐
                                      ▼                       ▼
                            [ Verify Network ]     [ Watchdog Timeout / Crash ]
                                      │                       │
                                      ▼                       ▼
                              (Write "Valid: 1")       (Hardware Reset)
                                      │                       │
                                      ▼                       ▼
                              [ Commit Boot ]      [ Bootloader Reverts to Bank A ]
```

* **The Mechanics:**
  1. The bootloader installs the new firmware to Bank B and boots it *provisionally*.
  2. A hardware watchdog timer is initialized on boot.
  3. The new application must run, perform a hardware self-test, successfully connect to the server, and write a confirmation flag (`Valid = 1`) to non-volatile memory.
  4. If the application crashes, hangs, or fails to reach the validation step before the timer expires, the hardware resets and the bootloader automatically reverts the boot pointer to Bank A.
* **The Benefit:** Eliminates manual physical recovery of field devices. A broken update will automatically roll back to the previously stable version, preserving device availability.
* **Disadvantages:**
  * **State Tracking Complexities:** The system must accurately distinguish between a soft crash (reboot loop) and a legitimate, temporary power loss during boot.
  * **Increased Recovery Latency:** Devices may remain in a non-functional loop for several minutes (depending on the rollback watchdog configuration) before the revert occurs.
