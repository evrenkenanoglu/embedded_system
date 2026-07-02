# ADVANCED OTA APPLICATION-LEVEL ARCHITECTURE: ADAPTIVE PULL-ON-TRIGGER

---

## 1. Transport & Trigger Matrix

| Network Profile            | Protocol                    | Trigger Mechanism                                         | Efficiency Vector                                                        | Disadvantages & Tradeoffs                                                                                                |
| :------------------------- | :-------------------------- | :-------------------------------------------------------- | :----------------------------------------------------------------------- | :----------------------------------------------------------------------------------------------------------------------- |
| **Cellular / Mains**       | CoAP / MQTT (Clean Session) | 1-bit flag (`update_available: 1`) in telemetry ACK.      | Eliminates dedicated polling connections; piggybacks on existing uplink. | High latency for critical patches; updates are gated entirely by the telemetry frequency.                                |
| **LPWAN (LoRaWAN/NB-IoT)** | Class A Downlink            | Cloud queues downlink to match ephemeral Rx1/Rx2 windows. | Zero-power standby; device radio remains off until signaled.             | Extreme payload constraints; downloading even small binaries takes substantial time due to duty-cycle limits.            |
| **Local (Wi-Fi/Ethernet)** | mDNS / UDP Broadcast        | Local edge-node service discovery advertisement.          | Bypasses WAN egress costs; executes local-speed distribution.            | Requires local hardware infrastructure; multicast traffic is frequently blocked by enterprise network security policies. |

---

## 2. Network Strategies & Sequence Flows

### Cellular & Mains-Powered (Piggybacked Telemetry)
This strategy eliminates dedicated polling cycles by coupling update checks directly with routine data transmissions.

```
Device                       Cloud Broker
  │                                │
  │─── 1. Publish Telemetry ──────>│
  │                                │ (Evaluate firmware version)
  │<── 2. ACK + [update_flag: 1] ──│
  │                                │
 (If 0, sleep immediately)
```

* **The Mechanics:** When a device wakes to send standard sensor data, the cloud broker appends a 1-bit boolean flag (`update_available: 1/0`) inside the network-level acknowledgment packet (ACK). 
* **The Benefit:** If the flag is `0`, the device immediately tears down the connection. There is no additional network handshake or payload overhead specifically for the update check; the check itself has near-zero overhead.
* **Disadvantages:**
  * **Delayed Propagation:** High latency for emergency or critical security patches. If a device only publishes telemetry once every 24 hours, it will not discover a critical update for up to 24 hours.
  * **Uplink Dependency:** If the device's sensor reading fails or the uplink packet is lost, the device never receives the corresponding downlink ACK containing the update flag.

### LPWAN & Battery-Constrained (Downlink Queuing)
Designed for ultra-low-power protocols (e.g., LoRaWAN Class A or NB-IoT) where devices sleep for long periods to achieve multi-year battery life.

```
Device (Class A)                      Network Server / Cloud
  │                                             │
  │ (Deep Sleep: Radio OFF)                     │ [ Queue Update Command ]
  │                                             │
  │─── 1. Send Uplink (Data) ──────────────────>│
  │                                             │
  │ [ Open Rx1/Rx2 Window ]                     │
  │<── 2. Push Queued Downlink (Wake Command) ──│ (Delivered within milliseconds)
  │ [ Radio OFF ]                               │
  │                                             │
 (Transition to active state for download)
```

* **The Mechanics:** In LPWAN architectures, the device radio is powered down to conserve energy, opening brief receive windows (Rx1, Rx2) only immediately after an uplink transmission. To target these devices, the cloud places the update notification command into a persistent downlink queue on the network server.
* **The Benefit:** The device never expends energy actively searching for updates. The network server automatically delivers the queued "wake up and pull" command during the device's naturally scheduled post-uplink Rx window.
* **Disadvantages:**
  * **Strict Payload Limitations:** LPWAN protocols have severely limited payload capacities (often less than 250 bytes per packet). Reconstructing a firmware binary requires extreme packet fragmentation and complex error-correction protocols.
  * **Downlink Packet Loss:** LPWAN downlink paths are highly prone to packet loss. If the wake-up command is dropped during the millisecond RX window, the network server must wait for the next uplink to retry.

### Local Networks (Edge Multicast / mDNS)
For enterprise, industrial, or healthcare deployments where devices reside on a shared local network, bypassing external WAN routing is critical.

```
Local Devices (LAN)                Edge Gateway / Local Server
  │                                             │
  │<── 1. UDP Multicast / mDNS Announcement ────│ (Announces: "v2.1.0 Available")
  │                                             │
  │─── 2. Local HTTP / CoAP GET ───────────────>│ (Request binary via local IP)
  │                                             │
  │<── 3. High-Speed Local Download ────────────│ (Bypasses WAN / Public Internet)
  │                                             │
```

* **The Mechanics:** An on-premise edge gateway or local server periodically broadcasts a lightweight UDP multicast packet or advertises its state using Multicast DNS (mDNS). 
* **The Benefit:** Local devices listen to this local advertisement to determine if their firmware is out of date. The subsequent download occurs entirely over the high-speed local intranet, eliminating external cellular data charges, bypassing public cloud egress fees, and securing the transfer within the local firewall.
* **Disadvantages:**
  * **Infrastructure Overhead:** Requires physical on-premises hardware (an edge server or gateway) to orchestrate and host the updates locally. If this edge node fails, the entire local subnet's update mechanism breaks.
  * **Network Administration Restrictions:** Enterprise and corporate Wi-Fi networks frequently block or rate-limit multicast traffic and UDP broadcasts by default to prevent broadcast storms, requiring manual network configuration and security exceptions.

---

## 3. Core Execution Policies

### Randomized Jitter (Anti-Congestion)
When a fleet of devices is configured to check for updates on a fixed schedule (e.g., every day at midnight), they create a massive concurrent traffic spike (the "thundering herd" problem) that can crash the hosting servers or CDNs.

* **The Formula:**
  $$\text{Next Connection Interval} = T_{\text{base}} \pm \text{RandomOffset}$$
* **The Explanation:** Adding a randomized time modifier (e.g., $\pm 30$ minutes) to the scheduling timer distributes the server request load evenly over a wider window, ensuring smooth, predictable resource utilization on the backend.
* **Disadvantages:**
  * **Unpredictable Update Completion Times:** Makes fleet-wide deployment verification difficult to track in real-time, as devices will apply the update at highly variable times.
  * **Debugging Complexity:** Troubleshooting connectivity logs becomes harder because there is no predictable time window when a specific device is expected to initiate its handshake.

### Binary Delta Compression (Differential Updates)
Transmitting a complete, monolithic firmware binary (often multiple megabytes) over low-bandwidth or metered connections is highly inefficient and prone to mid-transfer failures.

* **The Mechanics:** Instead of sending the full image, a differential algorithm (e.g., BSPatch or detools) compares the old binary with the new binary at compile time. It generates a "delta" file containing only the specific bytes that have changed. 
* **The Explanation:** The device downloads this highly compressed delta file (frequently reducing a 4MB payload down to 100KB) and uses an on-chip patching utility to reconstruct the new firmware version directly in its flash memory.
* **Disadvantages:**
  * **Computational Overhead:** The reconstruction process is memory and CPU-intensive. Resource-constrained microcontrollers may lack the RAM required to run the decompression algorithm.
  * **Dependency Chains:** Delta files are strictly version-dependent (e.g., a v1.0 $\rightarrow$ v1.2 patch cannot be applied to a v1.1 device). The release pipeline must generate, store, and manage a matrix of delta files for every historical firmware version currently active in the field.

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
  * **Perpetual Outdate Risks:** If a device's battery degrades and can never reach the required charging threshold (e.g., never climbs above the required 80%), or if it is installed in a permanently low-signal environment, the device will remain unpatched indefinitely without throwing an explicit network error.

To achieve a production-grade, highly resilient Over-the-Air (OTA) implementation, three critical architectural pillars are missing: **A/B Storage Partitioning**, **Cryptographic Integrity Validation (Secure Boot)**, and **Self-Test & Automated Rollback**. 

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
  * **Double Memory Footprint:** Requires exactly double the flash space for application code, which raises hardware Unit Costs.
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
  * **Anti-Downgrade:** The bootloader compares the incoming firmware version against a monotonic hardware counter stored in secure eFuses. 
* **The Benefit:** Prevents Man-in-the-Middle (MitM) attacks from pushing corrupted or malicious code, and blocks hackers from downgrading the firmware to an older version containing known vulnerabilities.
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