# ESP32 Provisioning & Release Automation Suite

A generic, data-driven toolchain for ESP-IDF projects to automate encrypted NVS generation, release code-signing, server manifest packaging, and factory-line silicon provisioning (Secure Boot V2, Flash Encryption, anti-rollback eFuses).

---

## Directory Structure

```text
Source/Scripts/provisioning/
├── config.yaml                    # Project configuration & environment paths (Single Source of Truth)
├── main.py                        # Unified CLI orchestrator
├── requirements.txt               # Python package dependencies
├── SKILLS.md                      # AI agent capabilities & operational reference
├── factory/
│   ├── audit_logger.py            # Generates per-unit JSON audit logs
│   ├── partition_parser.py        # Resolves flash offsets and sizes from partitions.csv
│   └── provision_hardware.py      # Flashes partitions and burns physical eFuses
├── nvs/
│   ├── partition_nvs_generator.py # Encrypted NVS partition binary generator
│   └── templates/
│       ├── nvs_cloud_template.csv  # AWS / Azure mTLS NVS template
│       ├── nvs_matter_template.csv # Matter DAC/PAI/CD credentials template
│       └── nvs_ota_template.csv    # Dual-phase OTA Trust Anchor & config template
└── signing/
    ├── hsm_sign_digest.py         # ECDSA SECP256R1 / RSA binary signer
    └── ota_manifest_packager.py   # Packages signed releases into manifest.json
```

---

## Prerequisites

* **Python:** 3.10 or higher
* **ESP-IDF Environment:** ESP-IDF v5.x+ (provides `esptool.py`, `espefuse.py`, and `nvs_partition_gen.py`)

### Installation

```bash
pip install -r Source/Scripts/provisioning/requirements.txt
```

---

## Configuration (`config.yaml`)

All project-specific paths and parameters are isolated inside `config.yaml`. The underlying Python scripts resolve all file paths dynamically using `{project_root_dir}` and `{embedded_system_dir}` variables.

```yaml
# Root Directory Variables
environment:
  project_root_dir: "."                           # Workspace Root (e.g., .../SMART_PLUGS/SW)
  embedded_system_dir: "{project_root_dir}/embedded_system"

# Paths & File Definitions
paths:
  partitions_csv: "{project_root_dir}/partitions.csv"
  output_dir: "{embedded_system_dir}/build/provisioning"
  firmware_dir: "{embedded_system_dir}/build"
  keys_dir: "{embedded_system_dir}/keys"
  certs_dir: "{embedded_system_dir}/certs"
  audit_dir: "{embedded_system_dir}/build/audit_logs"
  server_manifest: "{project_root_dir}/server/manifest.json"

# 1. Generic NVS Partition Generation
nvs_generation:
  target_partition: "fctry"
  template_csv: "{embedded_system_dir}/Source/Scripts/provisioning/nvs/templates/nvs_ota_template.csv"
  output_key_bin: "{embedded_system_dir}/build/provisioning/nvs_keys.bin"
  output_encrypted_bin: "{embedded_system_dir}/build/provisioning/nvs_encrypted.bin"
  template_variables:
    DEVICE_ID: "DEV-ESP32S3-001"
    HW_REV: "ESP32-S3-WROOM"
    HSVN: 1
    CHANNEL: "stable"
    API_KEY: "secure-device-token-factory-001"
    GATEWAY_URL: "https://ota.yourcompany.com:8443"
    ROOT_CA_PATH: "{embedded_system_dir}/certs/ca.crt"

# 2. Release & Code-Signing Pipeline
signing:
  version: "1.0.0"
  binary_name: "firmware.bin"
  key_type: "ec-secp256r1" # ec-secp256r1 | rsa-2048
  signing_key_pem: "{embedded_system_dir}/keys/signing.key"
  signing_cert_pem: "{embedded_system_dir}/certs/signing.crt"
  out_signature_bin: "{embedded_system_dir}/build/provisioning/target_signature.sig"
  manifest_metadata:
    channel: "stable"
    hardware_device: "ESP32-S3-WROOM"
    target_hsvn: 1
    canary_percentage: 100

# 3. Hardware Silicon Provisioning & Flashing
hardware:
  port: "COM3"
  baud: 460800
  chip: "esp32s3"
  flash_mode: "dio"
  flash_freq: "80m"
  flash_size: "16MB"
  dry_run: true
  force_burn: false

  efuse_keys:
    - block: "BLOCK_KEY0"
      key_file: "{embedded_system_dir}/keys/flash_encryption_key.bin"
      purpose: "FLASH_ENCRYPTION"
    - block: "BLOCK_KEY1"
      key_file: "{embedded_system_dir}/keys/secure_boot_digest.bin"
      purpose: "SECURE_BOOT_DIGEST0"

  efuse_registers:
    - name: "DIS_PAD_JTAG"
      value: "1"
    - name: "DIS_USB_JTAG"
      value: "1"
    - name: "DIS_DIRECT_BOOT"
      value: "1"
    - name: "DIS_DOWNLOAD_ICACHE"
      value: "1"
    - name: "DIS_DOWNLOAD_DCACHE"
      value: "1"
    - name: "SECURE_VERSION"
      value: "1"

  flash_targets:
    __bootloader__: "{embedded_system_dir}/build/bootloader.bin"
    __partition_table__: "{embedded_system_dir}/build/partition-table.bin"
    nvs_keys: "{embedded_system_dir}/build/provisioning/nvs_keys.bin"
    fctry: "{embedded_system_dir}/build/provisioning/nvs_encrypted.bin"
    ota_0: "{embedded_system_dir}/build/firmware.bin"
```

---

## How to Use

Commands below are executed from the top-level workspace root (`SW/`).

### 1. Execute Full Pipeline (NVS $\rightarrow$ Sign $\rightarrow$ Provision)

```bash
# Dry-run validation (does not burn silicon eFuses)
python embedded_system/Source/Scripts/provisioning/main.py \
    --config embedded_system/Source/Scripts/provisioning/config.yaml \
    --step all \
    --dry-run
```

```bash
# Physical silicon execution (requires 'hardware.force_burn: true' in config.yaml)
python embedded_system/Source/Scripts/provisioning/main.py \
    --config embedded_system/Source/Scripts/provisioning/config.yaml \
    --step all
```

---

### 2. Individual Step Execution

#### Step A: Generate Encrypted NVS Partition Only
Resolves the target partition size dynamically from `partitions.csv`, renders variables into `nvs_ota_template.csv`, generates a 64-byte AES-XTS key (`nvs_keys.bin`), and outputs the encrypted binary (`nvs_encrypted.bin`).

```bash
python embedded_system/Source/Scripts/provisioning/main.py \
    --config embedded_system/Source/Scripts/provisioning/config.yaml \
    --step nvs
```

#### Step B: Sign Firmware Release & Package Server Manifest
Computes the SHA-256 digest of `firmware.bin`, signs it using the configured private key (`signing.key`), outputs `target_signature.sig`, and inserts release metadata into `server/manifest.json`.

```bash
python embedded_system/Source/Scripts/provisioning/main.py \
    --config embedded_system/Source/Scripts/provisioning/config.yaml \
    --step sign
```

#### Step C: Factory Silicon Flashing & eFuse Burning
Reads target MAC address, burns configured eFuse keys and lock registers, resolves flashing offsets dynamically from `partitions.csv`, flashes all binaries, and writes an audit log to `build/audit_logs/audit_<MAC>.json`.

```bash
python embedded_system/Source/Scripts/provisioning/main.py \
    --config embedded_system/Source/Scripts/provisioning/config.yaml \
    --step provision \
    --dry-run
```

---

### 3. Standalone Script Usage

Each tool can be invoked independently from CI/CD pipelines or test jigs:

#### Standalone NVS Generation:
```bash
python embedded_system/Source/Scripts/provisioning/nvs/partition_nvs_generator.py \
    --template embedded_system/Source/Scripts/provisioning/nvs/templates/nvs_ota_template.csv \
    --out-csv embedded_system/build/provisioning/nvs_rendered.csv \
    --out-bin embedded_system/build/provisioning/nvs_encrypted.bin \
    --out-key embedded_system/build/provisioning/nvs_keys.bin \
    --partitions-csv partitions.csv \
    --target-partition fctry
```

#### Standalone Release Signing:
```bash
python embedded_system/Source/Scripts/provisioning/signing/hsm_sign_digest.py \
    --binary embedded_system/build/firmware.bin \
    --key embedded_system/keys/signing.key \
    --cert embedded_system/certs/signing.crt \
    --key-type ec-secp256r1 \
    --out-sig embedded_system/build/provisioning/firmware.sig \
    --out-json embedded_system/build/provisioning/release_meta.json
```

#### Standalone Partition Manifest Packager:
```bash
python embedded_system/Source/Scripts/provisioning/signing/ota_manifest_packager.py \
    --manifest server/manifest.json \
    --version 1.0.0 \
    --binary embedded_system/build/firmware.bin \
    --signature "<HEX_SIGNATURE>" \
    --signing-cert embedded_system/certs/signing.crt \
    --channel stable \
    --hsvn 1 \
    --hardware ESP32-S3-WROOM
```