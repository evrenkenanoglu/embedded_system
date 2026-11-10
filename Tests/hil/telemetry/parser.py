"""Host-side decoder for the serial telemetry stream.

Tiers: T0 raw lines, T1 ESP-IDF log lines and [HIL_REPORT] lines, T2 @TLM records (wire format v1).
"""

from __future__ import annotations

import re
import time
from dataclasses import dataclass

from telemetry.crc16 import crc16_ccitt_false
from telemetry.ids import (
    decode_categories,
    decode_severity,
    is_known_notification_id,
    notification_name,
)

MAX_TLM_LINE_BYTES = 120
MAX_PENDING_BYTES = 4096
U16_MAX = 0xFFFF
U32_MAX = 0xFFFFFFFF
TLM_MARKER = "@TLM"
CRC_SEPARATOR = " crc="
HIL_REPORT_MARKER = "[HIL_REPORT]"

_ANSI_ESCAPE = re.compile(r"\x1b\[[0-9;]*[A-Za-z]")
_LOG_LINE = re.compile(r"([EWIDV]) \(([0-9]+)\) ([^:]+): ?(.*)")
_TLM_RECORD = re.compile(
    r"@TLM v=1 seq=([0-9]+) ms=([0-9]+) kind=(STATE|EVENT) id=0x([0-9A-Fa-f]{2})"
    r" cat=0x([0-9A-Fa-f]{2}) sev=([IWEC]) act=([01]) pl=0x([0-9A-Fa-f]{8})"
    r" crc=0x([0-9A-Fa-f]{4})"
)
_TLM_DROP = re.compile(r"@TLM v=1 kind=DROP n=([0-9]+)")


@dataclass(frozen=True)
class RawLine:
    """T0: every line as received, with the host capture time."""

    host_time: float
    text: str


@dataclass(frozen=True)
class LogLine:
    """T1: ESP-IDF log line `L (ms) TAG: message`."""

    host_time: float
    level: str
    tag: str
    ms: int
    message: str


@dataclass(frozen=True)
class HilReportLine:
    """T1: `[HIL_REPORT] ...` text. Level, tag, and ms are None when the line has no ESP-IDF prefix."""

    host_time: float
    level: str | None
    tag: str | None
    ms: int | None
    body: str


@dataclass(frozen=True)
class TelemetryRecord:
    """T2: CRC-verified `@TLM` record."""

    host_time: float
    seq: int
    ms: int
    kind: str
    notification_id: int
    name: str
    category_mask: int
    categories: tuple[str, ...]
    severity: str
    active: bool
    payload: int
    raw: str


@dataclass(frozen=True)
class DropRecord:
    """T2: `@TLM v=1 kind=DROP n=<u32>`, the firmware's count of records it could not write."""

    host_time: float
    count: int


Record = RawLine | LogLine | HilReportLine | TelemetryRecord | DropRecord


@dataclass
class TelemetryCounters:
    records: int = 0
    corrupt_crc: int = 0
    malformed: int = 0
    oversize: int = 0
    dropped: int = 0
    gaps: int = 0
    lost: int = 0
    out_of_order: int = 0
    unknown_ids: int = 0

    @property
    def corrupt(self) -> int:
        return self.corrupt_crc + self.malformed + self.oversize


class TelemetryParser:
    """Stateful decoder. Feed complete lines with feed_line, or raw serial chunks with feed_chunk."""

    def __init__(self) -> None:
        self.counters = TelemetryCounters()
        self._last_seq: int | None = None
        self._pending = b""

    def feed_line(
        self, line: bytes | str, host_time: float | None = None
    ) -> list[Record]:
        timestamp = time.time() if host_time is None else host_time
        text = (
            line.decode("utf-8", errors="replace") if isinstance(line, bytes) else line
        )
        text = text.rstrip("\r\n")
        records: list[Record] = [RawLine(timestamp, text)]

        markers = [match.start() for match in re.finditer(TLM_MARKER, text)]
        if not markers:
            records.extend(self._parse_text(text, timestamp))
            return records

        if text[: markers[0]].strip():
            records.extend(self._parse_text(text[: markers[0]], timestamp))
        for start, end in zip(markers, markers[1:] + [len(text)]):
            record = self._parse_tlm(text[start:end].rstrip(), timestamp)
            if record is not None:
                records.append(record)
        return records

    def feed_chunk(self, data: bytes, host_time: float | None = None) -> list[Record]:
        timestamp = time.time() if host_time is None else host_time
        self._pending += data
        records: list[Record] = []
        while (newline := self._pending.find(b"\n")) >= 0:
            line, self._pending = self._pending[:newline], self._pending[newline + 1 :]
            records.extend(self.feed_line(line, timestamp))
        if len(self._pending) > MAX_PENDING_BYTES:
            line, self._pending = self._pending, b""
            records.extend(self.feed_line(line, timestamp))
        return records

    def flush(self, host_time: float | None = None) -> list[Record]:
        if not self._pending:
            return []
        line, self._pending = self._pending, b""
        return self.feed_line(line, host_time)

    def _parse_text(self, text: str, timestamp: float) -> list[Record]:
        plain = _ANSI_ESCAPE.sub("", text).strip()
        match = _LOG_LINE.fullmatch(plain)
        if match is not None:
            level, ms, tag, message = (
                match.group(1),
                int(match.group(2)),
                match.group(3),
                match.group(4),
            )
            if message.startswith(HIL_REPORT_MARKER):
                body = message[len(HIL_REPORT_MARKER) :].strip()
                return [HilReportLine(timestamp, level, tag, ms, body)]
            return [LogLine(timestamp, level, tag, ms, message)]
        if plain.startswith(HIL_REPORT_MARKER):
            body = plain[len(HIL_REPORT_MARKER) :].strip()
            return [HilReportLine(timestamp, None, None, None, body)]
        return []

    def _parse_tlm(self, segment: str, timestamp: float) -> Record | None:
        if len(segment.encode("utf-8")) > MAX_TLM_LINE_BYTES:
            self.counters.oversize += 1
            return None

        drop = _TLM_DROP.fullmatch(segment)
        if drop is not None:
            count = int(drop.group(1))
            if count > U32_MAX:
                self.counters.malformed += 1
                return None
            self.counters.dropped += count
            return DropRecord(timestamp, count)

        match = _TLM_RECORD.fullmatch(segment)
        if match is None:
            self.counters.malformed += 1
            return None
        seq_text, ms_text, kind, id_hex, cat_hex, sev, act, payload_hex, crc_hex = (
            match.groups()
        )
        seq, ms = int(seq_text), int(ms_text)
        if seq > U16_MAX or ms > U32_MAX:
            self.counters.malformed += 1
            return None

        crc_position = segment.rindex(CRC_SEPARATOR)
        if crc16_ccitt_false(segment[:crc_position].encode("ascii")) != int(
            crc_hex, 16
        ):
            self.counters.corrupt_crc += 1
            return None

        notification_id = int(id_hex, 16)
        if not is_known_notification_id(notification_id):
            self.counters.unknown_ids += 1
        self._track_sequence(seq)
        self.counters.records += 1

        category_mask = int(cat_hex, 16)
        return TelemetryRecord(
            host_time=timestamp,
            seq=seq,
            ms=ms,
            kind=kind,
            notification_id=notification_id,
            name=notification_name(notification_id),
            category_mask=category_mask,
            categories=decode_categories(category_mask),
            severity=decode_severity(sev),
            active=act == "1",
            payload=int(payload_hex, 16),
            raw=segment,
        )

    def _track_sequence(self, seq: int) -> None:
        last = self._last_seq
        self._last_seq = seq
        if last is None:
            return
        # Modular distance from the expected next value. The lower half of the u16 ring is a forward jump
        # (lost records); the upper half is a repeat or a reorder.
        distance = (seq - last - 1) & U16_MAX
        if distance == 0:
            return
        if distance < 0x8000:
            self.counters.gaps += 1
            self.counters.lost += distance
        else:
            self.counters.out_of_order += 1
