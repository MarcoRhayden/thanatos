from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class AppSettings:
    """
    Application bootstrap settings resolved from CLI/ENV.

    Notes:
    - CLI should override ENV
    - ENV should override defaults
    """

    config_path: Path
    thanatos_bin: Path
    delay_ms: int = 250
