# Generic QR Code Generator

Standalone, payload-agnostic tool. It turns arbitrary text into a QR image (PNG, optional SVG)
and appends a traceability record to a JSON Lines log. It has no knowledge of Wi-Fi, SoftAP or
the product. Payload formatting and label layout belong to the caller.

## Dependency

Requires `segno` (`requirements.in`). The tool does not install it.

## Invocation

Through Invoke (payload taken from an environment variable, never from argv):

```bash
export QR_PAYLOAD='WIFI:S:example;T:WPA;P:secret;;'
cd embedded_system
python -m invoke es.qr.generate --device-id=DEV-0001 --output=build/qr/DEV-0001.png \
    --log=build/qr/trace.jsonl --svg=build/qr/DEV-0001.svg
```

Standalone (for direct use):

```bash
printf '%s' "$PAYLOAD" | python Tools/QR/main.py --stdin --device-id DEV-0001 \
    --output out.png --log trace.jsonl
```

## Options

| Option | Default | Notes |
|---|---|---|
| `--ecc` | `M` | L, M, Q or H. Never boosted silently. |
| `--scale` | `10` | Pixels per module, 1-100. |
| `--border` | `4` | Quiet-zone modules, 0-100. |
| `--force` | off | Without it, existing output files are refused. |
| `--log` | required | Must not be an output path. Not configurable in YAML. |

Payloads over the byte-mode capacity for the chosen ECC level are refused with an error.
Lower the ECC level or shorten the payload.

## Traceability log

Each run appends one JSON line:

`device_id`, `timestamp_utc`, `tool`, `tool_version`, `payload_sha256`, `payload_bytes`,
`error_correction`, `scale`, `border`, `outputs[{format, file, sha256}]`.

The plaintext payload is never written. Note that SHA-256 of a short, low-entropy secret
(for example a Wi-Fi password) can be brute-forced if the log leaks. Keep the log access-controlled.

## Exit codes

- `0`: success.
- `1`: user or I/O error (message printed to stderr, payload not included).
- `2` (Invoke adapter): missing required argument or payload variable.
