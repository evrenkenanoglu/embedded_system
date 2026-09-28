### Prerequisites & Port Permissions

Before flashing, grant user read/write access to the USB serial interface:

```bash
sudo usermod -aG dialout $USER
sudo chmod 666 /dev/ttyUSB0
```

---

### Step 1: Generate SSoT Build Artifacts

Generate `partitions.csv`, `sdkconfig.hardware`, `project_version.cmake`, and `ota_generated_config.h` from `configs/config_project.yaml` [18]:

```bash
inv es.build-config.all
```

*Expected Output:*
```text
✅ Generated: .../partitions.csv
✅ Generated: .../sdkconfig.hardware
✅ Generated: .../project_version.cmake
✅ Generated: .../main/swConfig/ota_generated_config.h
```

---

### Step 2: Generate PKI Certificates and Silicon Security Keys

Create the Root CA, developer code-signing pair, server TLS credentials, and ESP32-S3 hardware encryption keys:

```bash
inv es.crypto.generate-pki
```

*Generated Files:*
* `certs/ca.crt` & `certs/ca.key` (Master Trust Anchor)
* `certs/signing.crt` & `certs/signing.key` (Developer Release Signer)
* `certs/server.crt` & `certs/server.key` (OTA Gateway HTTPS TLS)
* `keys/flash_encryption_key.bin` (256-bit AES-XTS key for `BLOCK_KEY0`) [11]
* `keys/secure_boot_signing_key.pem` & `keys/secure_boot_digest.bin` (Secure Boot V2 for `BLOCK_KEY1`) [11]

---

### Step 3: Build Baseline Firmware (`v1.0.0-dev1`)

Clean the build cache and compile the baseline firmware [16, 17]:

```bash
rm -rf build/ sdkconfig
idf.py build
```

*Output Verification:*
* `build/bootloader/bootloader.bin` (signed, size $\le$ `0x10000`) [7, 8]
* `build/partition_table/partition-table.bin` (offset `0x10000`) [8]
* `build/Embedded_IoT_BT_WIFI_Base_Project.bin`

---

### Step 4: Factory Silicon Provisioning (Physical Board)

> **WARNING:** Burning eFuses is permanent and irreversible (One-Time Programmable) [11, 13].
> For a synthetic dry-run first, omit `force_burn: true` or pass `--simulate` [13].

In `configs/config_provisioning.yaml`, set:
```yaml
hardware:
  dry_run: false
  force_burn: true
```

Execute factory provisioning:

```bash
inv es.crypto.provision-hardware
```

*Actions Executed:*
1. Generates 64-byte AES-XTS key (`build/provisioning/nvs_keys.bin`).
2. Generates encrypted factory NVS image (`build/provisioning/nvs_encrypted.bin`) containing `ca.crt` [4, 10].
3. Computes SHA-256 hash of `Embedded_IoT_BT_WIFI_Base_Project.bin` and signs it via developer key.
4. Burns `BLOCK_KEY0` (Flash Encryption) and applies read/write protection [11, 13].
5. Burns `BLOCK_KEY1` (Secure Boot V2) and applies write protection [11].
6. Burns `DIS_PAD_JTAG`, `DIS_USB_JTAG`, `DIS_DIRECT_BOOT`, and `SECURE_VERSION = 1`.
7. Flashes all partitions dynamically to physical offsets (`0x0`, `0x10000`, `0x34000`, `0x37000`, `0x50000`) [4, 7, 8].
8. Saves manufacturing audit log in `build/audit_logs/manufacturing_summary.csv`.

---

### Step 5: Verify Initial Hardware Boot

Open the serial console:

```bash
inv es.esp32.monitor
```

*Expected Serial Console Output:*
```text
I (120) boot: Checking secure boot...
I (125) secure_boot_v2: Secure boot verification succeeded
I (130) flash_encrypt: Flash encryption mode is RELEASE
...
I (310) app_init: Running app version: 1.0.0-dev1
I (315) OtaManager: Primary Root CA trust anchor loaded from storage (1240 bytes)
I (320) OtaManager: Active partition does not require verification (state: 0)
I (4100) wifi: Connected to UniverseHome, IP: 192.168.1.150
I (4120) OtaManager: Stage 1: Checking for updates at https://192.168.1.100:8443...
E (4150) OtaManager: Request rejected or communication failure during manifest query.
```
*(Communication fails as expected because the OTA server is not yet running)*.

---

### Step 6: Start the Secure OTA Server

Open a second terminal and boot the local HTTPS server:

```bash
inv es.ota.server
```

*Expected Server Output:*
```text
Starting Secure OTA Server at https://0.0.0.0:8443
Local IP Access Endpoint:   https://192.168.1.100:8443
Uvicorn running on https://0.0.0.0:8443 (Press CTRL+C to quit)
```

Web administrative dashboard is accessible at: `https://localhost:8443` (or `https://192.168.1.100:8443`).

---

### Step 7: Create and Publish Target Firmware Update (`v1.0.0-dev2`)

In `configs/config_project.yaml`, increment the version:

```yaml
project:
  version: "1.0.0-dev2"
```

Compile, sign, and publish the update using the automated release task:

```bash
# 1. Regenerate build configurations with new version string
inv es.build-config.all

# 2. Build, sign, and package release directly into the OTA server manifest
inv es.ota.release
```

*Expected Output:*
```text
[*] [OTA RELEASE: STEP 1/2] Compiling firmware for target 'esp32s3'...
...
[*] [OTA RELEASE: STEP 2/2] Signing release binary and packaging manifest...
[*] Binary SHA-256 Digest: 8f4e2...
[*] Target Signature (ec-secp256r1): 304502...
[OK] Release v1.0.0-dev2 successfully packaged into: .../manifest.json
[SUCCESS] OTA release compilation, code-signing, and catalog update completed.
```

---

### Step 8: Observe Live Over-The-Air Update on Device

Switch back to the terminal running `inv es.esp32.monitor`:

```text
=================================================
Starting Professional Dual-Phase OTA Test Routine
=================================================
I (15400) OtaManager: Executing Phase 1: Gateway manifest handshake update evaluation...
I (15450) OtaManager: Stage 1 complete. Target Version: 1.0.0-dev2, Type: full, Size: 1845120
I (15460) OtaManager: Executing Phase 2 & 3: Direct stream download and automated telemetry logging...
I (15470) OtaService: [PKI] Certificate chain verified against primary Root CA.
I (15480) OtaService: [PKI] Dynamic signing certificate validated successfully against trust anchor.
I (15490) OtaHttpTransport: Appended HTTP Range header: Range: bytes=0-
I (15500) mem_ota: Writing to partition subtype 17 at offset 0x3d0000
I (15520) mem_ota: Firmware header validated successfully. Version: 1.0.0-dev2, Secure Version: 1
I (17200) OtaManager: Progress: 921600/1845120 bytes (50.00%)
I (19400) OtaManager: Progress: 1845120/1845120 bytes (100.00%)
I (19410) OtaService: [PKI] Calculating SHA-256 target partition digest...
I (19650) OtaService: [PKI] Verifying cryptographic signature against public key context...
I (19660) MbedTlsCryptoEngine: Signature check validation succeeded.
I (19670) mem_ota: Boot partition configured to new target. Ready for reset.
I (19680) OtaManager: Telemetry state reported successfully: success
I (21700) OtaManager: Executing platform soft reboot via callback...
```

---

### Step 9: Observe Provisional Boot & Self-Healing Execution

After soft reboot, the bootloader boots into Bank B (`ota_1` @ `0x3d0000`) in `PendingVerify` trial state [7, 8]:

```text
I (120) boot: Checking secure boot...
I (125) secure_boot_v2: Secure boot verification succeeded
I (130) boot: Loaded app from partition at offset 0x3d0000
...
I (310) app_init: Running app version: 1.0.0-dev2
I (320) OtaManager: Provisional boot detected (PendingVerify). Executing self-test diagnostic suite...
I (325) OtaManager: [SELF-TEST] Running check: 'NVS_Storage_Check'...
I (330) OtaManager: [SELF-TEST PASSED] 'NVS_Storage_Check'
I (335) OtaManager: [SELF-TEST] Running check: 'Network_Transport_Check'...
I (4120) OtaManager: [SELF-TEST PASSED] 'Network_Transport_Check'
I (4125) OtaManager: All provisional self-tests passed successfully. Committing partition (cancelling rollback)...
I (4130) mem_ota: App marked as valid, rollback cancelled successfully
I (4135) OtaManager: Partition successfully committed as permanently valid.
```

The device is now running `v1.0.0-dev2` with rollback cancelled.
