#!/usr/bin/env python3
"""Verify provisioned NVS key and encrypted partition artifacts without device I/O."""

import argparse
import csv
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path
from typing import Any, Dict, Iterable, Set, Tuple

import yaml

SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))

from factory.partition_parser import PartitionTableParser
from nvs.partition_nvs_generator import find_nvs_partition_gen_tool, render_template_csv


def _required_nvs_entries(csv_path: Path) -> Set[Tuple[str, str]]:
    entries: Set[Tuple[str, str]] = set()
    namespace = ""
    with csv_path.open("r", encoding="utf-8", newline="") as source:
        for row in csv.reader(source):
            if not row or row[0].strip().startswith("#") or row[0].strip() == "key":
                continue
            key = row[0].strip()
            entry_type = row[1].strip() if len(row) > 1 else ""
            if entry_type == "namespace":
                namespace = key
            elif namespace:
                entries.add((namespace, key))
    return entries


def _run_tool(command: Iterable[str]) -> None:
    result = subprocess.run(list(command), capture_output=True, text=True, check=False)
    if result.returncode:
        details = result.stderr.strip() or result.stdout.strip()
        raise RuntimeError(f"ESP-IDF NVS tool failed: {details}")


def verify(config_path: Path) -> None:
    with config_path.open("r", encoding="utf-8") as source:
        config: Dict[str, Any] = yaml.safe_load(source)

    paths = config["paths"]
    nvs = config["nvs_generation"]
    workspace = Path(paths["workspace_dir"]).resolve()
    key_path = Path(nvs["output_key_bin"]).resolve()
    key_partition_path = Path(nvs["output_key_partition_bin"]).resolve()
    encrypted_path = Path(nvs["output_encrypted_bin"]).resolve()
    template_path = Path(nvs["template_csv"]).resolve()
    partitions_path = Path(paths["partitions_csv"]).resolve()
    target_partition = str(nvs["target_partition"])
    key_size = int(nvs["key_size_bytes"])

    for path in (
        key_path,
        key_partition_path,
        encrypted_path,
        template_path,
        partitions_path,
    ):
        if not path.is_file():
            raise FileNotFoundError(
                f"Required NVS verification input is missing: {path}"
            )

    key = key_path.read_bytes()
    if len(key) != key_size:
        raise ValueError(
            f"NVS key size mismatch: expected {key_size} bytes, found {len(key)}"
        )

    expected_key_partition = key + zlib.crc32(key).to_bytes(4, "little") + b"\xff" * 12
    if key_partition_path.read_bytes() != expected_key_partition:
        raise ValueError(
            "NVS key partition image does not match key, CRC, and erased padding"
        )

    partition_table = PartitionTableParser(partitions_path)
    key_partition_size = partition_table.get_size("nvs_keys")
    if len(expected_key_partition) > key_partition_size:
        raise ValueError("NVS key image is larger than the nvs_keys partition")
    partition_size = partition_table.get_size(target_partition)
    encrypted_image = encrypted_path.read_bytes()
    if len(encrypted_image) != partition_size:
        raise ValueError(
            f"Encrypted NVS image size mismatch: expected {partition_size} bytes, "
            f"found {len(encrypted_image)}"
        )

    variables = nvs.get("template_variables", {})
    required_entries = {
        ("sec_pki", "root_ca_pem"),
        ("sec_pki", "ca_backup_pem"),
    }
    template_entries = _required_nvs_entries(template_path)
    if not required_entries.issubset(template_entries):
        raise ValueError(
            "NVS template is missing required primary/backup Root CA entries"
        )

    for variable_name in ("ROOT_CA_PATH", "ROOT_CA_BACKUP_PATH"):
        cert_path = Path(str(variables[variable_name])).resolve()
        if not cert_path.is_file():
            raise FileNotFoundError(f"Configured Root CA input is missing: {cert_path}")

    tool = find_nvs_partition_gen_tool()
    if not tool:
        raise RuntimeError("Could not locate the ESP-IDF nvs_partition_gen tool")

    with tempfile.TemporaryDirectory(prefix="verify-nvs-") as temp_name:
        temp_dir = Path(temp_name)
        rendered_template = temp_dir / "nvs.csv"
        decrypted_image = temp_dir / "decrypted.bin"
        expected_plaintext = temp_dir / "expected.bin"

        render_template_csv(template_path, rendered_template, variables, workspace)
        if not required_entries.issubset(_required_nvs_entries(rendered_template)):
            raise ValueError(
                "Rendered NVS template is missing required Root CA entries"
            )

        _run_tool(
            tool
            + [
                "decrypt",
                str(encrypted_path),
                str(key_path),
                str(decrypted_image),
                "--outdir",
                str(temp_dir),
            ]
        )
        _run_tool(
            tool
            + [
                "generate",
                str(rendered_template),
                str(expected_plaintext),
                hex(partition_size),
            ]
        )

        if decrypted_image.read_bytes() != expected_plaintext.read_bytes():
            raise ValueError(
                "Encrypted NVS image does not decrypt to the configured template; "
                "key and image may be mismatched"
            )

    print("NVS artifact verification passed:")
    print(f"  key size and key-partition CRC/padding: valid ({key_size} bytes)")
    print(f"  encrypted partition: valid ({target_partition}, {partition_size} bytes)")
    print("  decrypted contents: match configured template and both Root CA entries")
    print("  device flash/eFuse operations: not performed")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--config", type=Path, required=True, help="Resolved provisioning YAML"
    )
    args = parser.parse_args()
    try:
        verify(args.config.resolve())
    except Exception as error:
        print(f"NVS verification failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
