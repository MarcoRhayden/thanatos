from __future__ import annotations

from pathlib import Path
from typing import Protocol, Optional


class RunningProcess(Protocol):
    """Process handle used by the orchestrator."""

    pid: int

    def poll(self) -> Optional[int]:
        ...


class ProcessRunner(Protocol):
    """Process start/stop abstraction."""

    def start(self, server_bin: Path, config_path: Path, cwd: Path) -> RunningProcess:
        ...

    def terminate_process_group(self, proc: RunningProcess, timeout: float) -> None:
        ...
