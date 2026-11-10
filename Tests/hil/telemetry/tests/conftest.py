"""Makes the `telemetry` package importable from the parent `Tests/hil` directory."""

import sys
from pathlib import Path

HIL_DIR = Path(__file__).resolve().parents[2]
if str(HIL_DIR) not in sys.path:
    sys.path.insert(0, str(HIL_DIR))
