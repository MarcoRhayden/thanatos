from __future__ import annotations

import atexit
from pathlib import Path

from ...application.ports.history_store import HistoryStore

try:
    import readline  # type: ignore
except Exception:
    readline = None


class ReadlineHistoryStore(HistoryStore):
    """Persists command history so Up/Down arrows work across runs."""

    def setup(self, history_file: Path, max_len: int) -> None:
        if readline is None:
            return

        history_file.parent.mkdir(parents=True, exist_ok=True)

        try:
            if history_file.exists():
                readline.read_history_file(str(history_file))
            readline.set_history_length(int(max_len))
        except Exception:
            return

        def save() -> None:
            try:
                readline.set_history_length(int(max_len))
                readline.write_history_file(str(history_file))
            except Exception:
                pass

        atexit.register(save)
