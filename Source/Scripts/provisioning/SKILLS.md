## File: `Scripts/provisioning/skills.md`

# AI Agent Operational Skills: ESP32 Provisioning Engine

This document defines the architectural context, operational invariants, and command interfaces for AI coding assistants automating tasks within the provisioning framework.

---

## 1. System Knowledge & Architectural Invariants

### Flash & Partition Resolution
* **Partition Table:** The file `partitions.csv` is the single source of truth for the physical flash map. Never hardcode partition offsets in scripts or documentation.
* **Alignment Rules:**
  * Application partitions (`type: app`) require 64 KB alignment (`0x10000`).
  * Data partitions (`type: data`) require 4 KB sector alignment (`0x1000`).
  * Partition table always resides at `0x8000`. Bootloader always resides at `0x0000` (ESP32-S3).
* **Parser Engine:** `Scripts/provisioning/factory/partition_parser.py` resolves offsets and sizes dynamically. Use `pt_parser.get_offset(name)` and `pt_parser.get_size(name)`.

### Cryptographic Invariants
* **NVS Encryption:** Uses AES-XTS with a 64-byte key binary (32 bytes AES encryption key + 32 bytes XTS tweak key).
* **Firmware Signing:** 
  * ECDSA SECP256R1 signatures are generated over raw 32-byte SHA-256 digests.
  * Signature format is IEEE P1363 (raw $R \parallel S$, 64 bytes total), hex-encoded for server manifests.
* **Silicon Locks:** 
  * Burning eFuses is irreversible (One-Time Programmable).
  * Always verify `--dry-run` or configuration safety checks before executing on real silicon.

---

## 2. AI Operational Workflows

### Workflow A: Add a New NVS Variable
1. Update or create a template inside `Scripts/provisioning/nvs/templates/`.
2. Add the variable mapping to `config.yaml` under `nvs_generation.template_variables`.
3. Do **not** modify `partition_nvs_generator.py` or `main.py`.

### Workflow B: Add a New eFuse Key or Lock Register
1. Add the entry to `config.yaml` under `hardware.efuse_keys` or `hardware.efuse_registers`.
2. Ensure the key binary exists in `keys/`.
3. Execute `python Scripts/provisioning/main.py --step provision --dry-run` to validate syntax and block allocation.

### Workflow C: Adapt to a New `partitions.csv`
1. Update `partitions.csv` in the project root.
2. If the factory NVS partition name changed, update `nvs_generation.target_partition` and `hardware.flash_targets` in `config.yaml`.
3. Run `python Scripts/provisioning/main.py --step all --dry-run`. All offsets and partition binary sizes will adjust automatically.

---

## 3. Command Reference for AI Execution

| Task                                   | Command                                                                                                                                                                                                                     |
| :------------------------------------- | :-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Validate Full Pipeline (Dry-Run)**   | `python Scripts/provisioning/main.py --step all --dry-run`                                                                                                                                                                  |
| **Generate Encrypted NVS Only**        | `python Scripts/provisioning/main.py --step nvs`                                                                                                                                                                            |
| **Sign Firmware & Update Manifest**    | `python Scripts/provisioning/main.py --step sign`                                                                                                                                                                           |
| **Execute Real Hardware Flashing**     | `python Scripts/provisioning/main.py --step provision`                                                                                                                                                                      |
| **Parse Partition Offsets (CLI Test)** | `python -c "from factory.partition_parser import PartitionTableParser; from pathlib import Path; p=PartitionTableParser(Path('partitions.csv')); print([(k, hex(v.offset), hex(v.size)) for k,v in p.partitions.items()])"` |

---

## 4. Error Handling & Validation Rules

* **Missing Keys/Files:** If an input file is missing, the tool raises a clean `FileNotFoundError` before initiating serial communication.
* **eFuse Safety Guard:** If `dry_run: false` and `force_burn: false`, `main.py` terminates immediately with exit code `1` to prevent accidental silicon locking.
* **Audit Enforcement:** Every provisioned unit must generate an audit JSON record inside `build/audit_logs/` containing the burned key names, MAC address, and timestamp.