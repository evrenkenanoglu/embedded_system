# AP Password Tool (per-device SoftAP passphrases)

Standalone, stdlib-only tool that generates a unique WPA2/WPA3 SoftAP passphrase per device
at **manufacturing time** and writes a JSON provisioning record. It is independent of the
task-runner core and the project SSoT.

## Usage

Preferred (from the workspace root, via the SSoT task adapter):

```
inv es.build-config.ap-password --device-id DEV0001 --output-dir <dir outside workspace>
```

The default output directory is `~/.embedded_iot_provisioning/ap_passwords`. Records are refused
if the directory resolves inside the workspace.

Direct invocation of the tool:

```
python Tools/HW_SECURITY/ap_password/main.py --device-id DEV0001 --output <file.json> [--length 16] [--force]
```

Or through the tool's own Invoke task: `inv tools.ap-password.generate --device-id ... --output ...`.

## Properties

- Randomness: `secrets` (CSPRNG). Length 8..63 (default 16).
- Alphabet excludes ambiguous glyphs (`0 O 1 l I o`). At least one upper, lower and digit.
- Record: `{schema_version, device_id, ap_password, created_utc}`, written with mode `0600`
  inside a `0700` directory. An existing file is never overwritten unless `--force` is given.
- The password is never printed or logged; only the device ID and output path are.

## Factory-NVS integration

The project SSoT selects `per_device`. Generate the owner-only record outside the workspace using
the exact `DEVICE_ID` configured for the provisioning image, then pass it to both NVS generation
and verification:

```
inv es.build-config.ap-password --device-id DEV-esp32c6-001 --output-dir ~/.embedded_iot_provisioning/ap_passwords
inv es.crypto.provision-nvs --ap-password-record ~/.embedded_iot_provisioning/ap_passwords/DEV-esp32c6-001.json
inv es.crypto.verify-nvs --ap-password-record ~/.embedded_iot_provisioning/ap_passwords/DEV-esp32c6-001.json
```

The provisioning tools reject records with unsafe file permissions, invalid passphrases, or a
device ID that does not match the provisioning config. They create a mode-0600 temporary text file
only while rendering and generating/verifying encrypted factory NVS, then remove it. The firmware
loads the passphrase from the configured encrypted factory-NVS partition before starting Wi-Fi and
fails closed if the value is absent or invalid; it does not embed a shared password in the image.

Tests: `python -m unittest test_engine` (run from this directory).
