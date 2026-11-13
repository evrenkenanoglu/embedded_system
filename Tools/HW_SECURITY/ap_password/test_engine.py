"""Unit tests for the per-device SoftAP password tool. Run: python -m unittest test_engine"""

import json
import os
import stat
import tempfile
import unittest
from pathlib import Path

from engine import (
    AMBIGUOUS_GLYPHS,
    MAX_LENGTH,
    MIN_LENGTH,
    PASSWORD_ALPHABET,
    build_record,
    generate_password,
    load_password_record,
    validate_device_id,
    write_record,
    write_password_file,
)


class GeneratePasswordTests(unittest.TestCase):
    def test_length_bounds_are_enforced(self):
        self.assertEqual(len(generate_password(MIN_LENGTH)), MIN_LENGTH)
        self.assertEqual(len(generate_password(MAX_LENGTH)), MAX_LENGTH)
        with self.assertRaises(ValueError):
            generate_password(MIN_LENGTH - 1)
        with self.assertRaises(ValueError):
            generate_password(MAX_LENGTH + 1)

    def test_password_uses_only_unambiguous_alphabet(self):
        for _ in range(200):
            password = generate_password(63)
            self.assertTrue(set(password).issubset(set(PASSWORD_ALPHABET)))
            self.assertFalse(set(password) & AMBIGUOUS_GLYPHS)

    def test_password_contains_upper_lower_and_digit(self):
        for _ in range(200):
            password = generate_password(8)
            self.assertTrue(any(c.isupper() for c in password))
            self.assertTrue(any(c.islower() for c in password))
            self.assertTrue(any(c.isdigit() for c in password))


class DeviceIdTests(unittest.TestCase):
    def test_rejects_invalid_device_ids(self):
        for bad in ("", "-leading", "has space", "path/traversal", "x" * 65):
            with self.assertRaises(ValueError, msg=bad):
                validate_device_id(bad)

    def test_accepts_valid_device_id(self):
        self.assertEqual(validate_device_id("DEV-01_a"), "DEV-01_a")


class WriteRecordTests(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(dir=Path.cwd())
        self.dir = Path(self._tmp.name)

    def tearDown(self):
        self._tmp.cleanup()

    def test_record_is_written_with_owner_only_permissions(self):
        path = write_record(self.dir / "rec.json", build_record("DEV01"))
        data = json.loads(path.read_text(encoding="utf-8"))
        self.assertEqual(data["device_id"], "DEV01")
        self.assertEqual(len(data["ap_password"]), 16)
        self.assertEqual(stat.S_IMODE(os.stat(path).st_mode), 0o600)

    def test_existing_record_is_not_overwritten_without_force(self):
        target = self.dir / "rec.json"
        write_record(target, build_record("DEV01"))
        with self.assertRaises(FileExistsError):
            write_record(target, build_record("DEV01"))
        write_record(target, build_record("DEV01"), force=True)

    def test_password_record_is_device_bound_and_owner_only(self):
        record_path = write_record(self.dir / "rec.json", build_record("DEV01"))
        record = load_password_record(record_path, "DEV01")
        self.assertEqual(len(record), 16)
        with self.assertRaisesRegex(ValueError, "device ID"):
            load_password_record(record_path, "DEV02")

    def test_temporary_password_file_has_owner_only_mode_and_no_newline(self):
        path = write_password_file(self.dir / "password.txt", "Abcdefg2")
        self.assertEqual(stat.S_IMODE(os.stat(path).st_mode), 0o600)
        self.assertEqual(path.read_bytes(), b"Abcdefg2")

    def test_rejects_group_readable_record(self):
        record_path = write_record(self.dir / "rec.json", build_record("DEV01"))
        os.chmod(record_path, 0o640)
        with self.assertRaisesRegex(ValueError, "owner-only"):
            load_password_record(record_path, "DEV01")

    def test_refuses_insecure_record_directory(self):
        public_dir = self.dir / "public"
        public_dir.mkdir(mode=0o755)
        public_dir.chmod(0o755)
        with self.assertRaisesRegex(PermissionError, "directory must be private"):
            write_record(public_dir / "rec.json", build_record("DEV01"))


if __name__ == "__main__":
    unittest.main()
