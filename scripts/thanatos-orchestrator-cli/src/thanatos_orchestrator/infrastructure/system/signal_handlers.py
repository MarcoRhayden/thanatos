from __future__ import annotations

import signal
from typing import Callable


def install_signal_handlers(stop_all: Callable[[str], None]) -> None:
    """Install SIGINT/SIGTERM handlers to stop all servers before exiting."""

    def handler(signum, frame) -> None:
        try:
            stop_all(f"signal {signum}")
        finally:
            raise KeyboardInterrupt

    signal.signal(signal.SIGINT, handler)
    signal.signal(signal.SIGTERM, handler)
