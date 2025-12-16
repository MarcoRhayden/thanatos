from __future__ import annotations

from pathlib import Path
from typing import Protocol


class HistoryStore(Protocol):
    """CLI history persistence abstraction."""

    def setup(self, history_file: Path, max_len: int) -> None:
        ...
