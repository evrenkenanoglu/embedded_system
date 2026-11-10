"""Golden-vector tests for the host telemetry decoder (T0, T1, T2, DROP, sequence, ids)."""

import binascii
import re
from pathlib import Path

import pytest

from telemetry.crc16 import crc16_ccitt_false
from telemetry.ids import (
    FRAMEWORK_ID_NAMES,
    decode_categories,
    decode_severity,
)
from telemetry.parser import (
    DropRecord,
    HilReportLine,
    LogLine,
    RawLine,
    TelemetryParser,
    TelemetryRecord,
)

NOTIFICATION_TYPES_HPP = (
    Path(__file__).resolve().parents[4]
    / "Source"
    / "App"
    / "Notification"
    / "NotificationTypes.hpp"
)

GOLDEN_STATE_LINE = "@TLM v=1 seq=42 ms=10432 kind=STATE id=0x21 cat=0x02 sev=W act=1 pl=0x00000000 crc=0x2E2E"


def tlm_line(seq: int, notification_id: int = 0x21, ms: int = 10432) -> str:
    """Builds a valid record. The CRC comes from binascii (an independent oracle), not from the module under test."""
    body = f"@TLM v=1 seq={seq} ms={ms} kind=STATE id=0x{notification_id:02X} cat=0x02 sev=W act=1 pl=0x00000000"
    return f"{body} crc=0x{binascii.crc_hqx(body.encode('ascii'), 0xFFFF):04X}"


def of_type(records, record_type):
    return [record for record in records if isinstance(record, record_type)]


@pytest.fixture
def parser() -> TelemetryParser:
    return TelemetryParser()


def test_crc16_ccitt_false_check_value():
    assert crc16_ccitt_false(b"123456789") == 0x29B1


def test_valid_record_decodes_golden_vector(parser):
    records = parser.feed_line(GOLDEN_STATE_LINE, host_time=1.5)

    assert isinstance(records[0], RawLine)
    assert records[0].host_time == 1.5
    (record,) = of_type(records, TelemetryRecord)
    assert record.seq == 42
    assert record.ms == 10432
    assert record.kind == "STATE"
    assert record.notification_id == 0x21
    assert record.name == "WIFI_CONNECTING"
    assert record.categories == ("CONNECTIVITY",)
    assert record.severity == "WARNING"
    assert record.active is True
    assert record.payload == 0
    assert parser.counters.records == 1
    assert parser.counters.corrupt == 0


def test_corrupted_crc_and_payload_are_rejected(parser):
    bad_crc = GOLDEN_STATE_LINE.replace("crc=0x2E2E", "crc=0x2E2F")
    bad_payload = GOLDEN_STATE_LINE.replace("pl=0x00000000", "pl=0x00000001")

    assert of_type(parser.feed_line(bad_crc), TelemetryRecord) == []
    assert of_type(parser.feed_line(bad_payload), TelemetryRecord) == []
    assert parser.counters.corrupt_crc == 2
    assert parser.counters.records == 0


def test_truncated_line_is_malformed(parser):
    truncated = GOLDEN_STATE_LINE[: GOLDEN_STATE_LINE.index(" pl=")]

    assert of_type(parser.feed_line(truncated), TelemetryRecord) == []
    assert parser.counters.malformed == 1
    assert parser.counters.corrupt_crc == 0


def test_overlong_line_is_rejected(parser):
    overlong = GOLDEN_STATE_LINE + " " + "X" * 120

    assert of_type(parser.feed_line(overlong), TelemetryRecord) == []
    assert parser.counters.oversize == 1


def test_marker_resyncs_after_log_text_on_same_line(parser):
    records = parser.feed_line("I (10440) APP: hello" + GOLDEN_STATE_LINE)

    (log,) = of_type(records, LogLine)
    assert (log.level, log.tag, log.ms, log.message) == ("I", "APP", 10440, "hello")
    (record,) = of_type(records, TelemetryRecord)
    assert record.seq == 42


def test_record_split_by_interleaved_log_line_is_lost_and_shows_as_gap(parser):
    full = tlm_line(seq=7)
    split_at = full.index(" cat=")
    lines = [
        tlm_line(seq=6),
        full[:split_at],
        "I (10441) WIFI: connected",
        full[split_at:],
        tlm_line(seq=8),
    ]

    records = [record for line in lines for record in parser.feed_line(line)]

    assert [r.seq for r in of_type(records, TelemetryRecord)] == [6, 8]
    (log,) = of_type(records, LogLine)
    assert log.message == "connected"
    assert parser.counters.malformed == 1
    assert parser.counters.gaps == 1
    assert parser.counters.lost == 1


def test_drop_records_are_decoded_and_summed(parser):
    first = parser.feed_line("@TLM v=1 kind=DROP n=17")
    second = parser.feed_line("@TLM v=1 kind=DROP n=3")

    assert of_type(first, DropRecord)[0].count == 17
    assert of_type(second, DropRecord)[0].count == 3
    assert parser.counters.dropped == 20


def test_sequence_gap_is_detected_across_u16_wrap(parser):
    for seq in (65534, 65535, 0, 2):
        parser.feed_line(tlm_line(seq=seq))

    assert parser.counters.gaps == 1
    assert parser.counters.lost == 1
    assert parser.counters.out_of_order == 0


def test_repeated_sequence_is_out_of_order_not_a_gap(parser):
    parser.feed_line(tlm_line(seq=10))
    parser.feed_line(tlm_line(seq=10))

    assert parser.counters.gaps == 0
    assert parser.counters.out_of_order == 1


def test_unknown_framework_id_is_counted_and_app_id_is_not(parser):
    unknown = of_type(
        parser.feed_line(tlm_line(seq=1, notification_id=0x07)), TelemetryRecord
    )
    app = of_type(
        parser.feed_line(tlm_line(seq=2, notification_id=0x90)), TelemetryRecord
    )

    assert unknown[0].name == "UNKNOWN_0x07"
    assert app[0].name == "APP_0x90"
    assert parser.counters.unknown_ids == 1


def test_t1_log_and_hil_report_lines(parser):
    error = of_type(
        parser.feed_line("\x1b[0;31mE (1234) WIFI: connect failed\x1b[0m"), LogLine
    )[0]
    report = of_type(
        parser.feed_line("I (99) HIL: [HIL_REPORT] TEST: sys_rng_contract -> PASS"),
        HilReportLine,
    )[0]
    bare = of_type(
        parser.feed_line("[HIL_REPORT] END_HAL_REGRESSION: 5 PASSED, 0 FAILED"),
        HilReportLine,
    )[0]

    assert (error.level, error.tag, error.ms, error.message) == (
        "E",
        "WIFI",
        1234,
        "connect failed",
    )
    assert (report.level, report.tag, report.ms) == ("I", "HIL", 99)
    assert report.body == "TEST: sys_rng_contract -> PASS"
    assert (bare.level, bare.tag, bare.ms) == (None, None, None)


def test_feed_chunk_reassembles_lines_split_across_reads(parser):
    data = (tlm_line(seq=1) + "\n").encode("ascii")
    midpoint = len(data) // 2

    assert of_type(parser.feed_chunk(data[:midpoint]), TelemetryRecord) == []
    records = parser.feed_chunk(data[midpoint:])

    assert [r.seq for r in of_type(records, TelemetryRecord)] == [1]
    assert parser.flush() == []


def test_category_mask_and_severity_decoding():
    assert decode_categories(0x03) == ("SYSTEM", "CONNECTIVITY")
    assert decode_categories(0x41) == ("SYSTEM", "UNKNOWN_0x40")
    assert decode_severity("C") == "CRITICAL"


def _parse_framework_enum(header_text: str) -> dict[str, int]:
    text = re.sub(r"/\*.*?\*/", "", header_text, flags=re.DOTALL)
    text = re.sub(r"//[^\n]*", "", text)
    match = re.search(
        r"enum class framework_notification_id_t\s*:\s*uint8_t\s*\{(.*?)\};",
        text,
        re.DOTALL,
    )
    assert (
        match is not None
    ), "framework_notification_id_t not found in NotificationTypes.hpp"

    values: dict[str, int] = {}
    next_value = 0
    for entry in match.group(1).split(","):
        name, _, explicit = entry.partition("=")
        if not name.strip():
            continue
        if explicit.strip():
            next_value = int(explicit.strip(), 0)
        values[name.strip()] = next_value
        next_value += 1
    return values


def test_id_table_matches_notification_types_header():
    enum_values = _parse_framework_enum(
        NOTIFICATION_TYPES_HPP.read_text(encoding="utf-8")
    )

    expected = {name: value for value, name in FRAMEWORK_ID_NAMES.items()}
    assert expected == enum_values
