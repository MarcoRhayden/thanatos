from __future__ import annotations

import time
from dataclasses import dataclass
from pathlib import Path
from typing import Protocol, runtime_checkable, Optional

from ..enums.server_status import ServerStatus
from .port_triple import PortTriple


@runtime_checkable
class RunningProcess(Protocol):
    """Minimum surface area required from a running process handle."""

    pid: int

    def poll(self) -> Optional[int]:
        ...

    def terminate(self) -> None:
        ...


@runtime_checkable
class PortChecker(Protocol):
    """Port checker contract used to compute listening state."""

    def are_ports_listening(self, ports: list[int]) -> bool:
        ...


@dataclass(slots=True)
class ServerInstance:
    """Represents a single Thanatos instance tracked by the orchestrator."""

    block: int
    slot: int
    proc: RunningProcess
    ports: PortTriple
    server_name: str
    config_path: Path
    started_at: float = 0.0

    def __post_init__(self) -> None:
        if not self.started_at:
            self.started_at = time.time()

    def proc_running(self) -> bool:
        try:
            return self.proc.poll() is None
        except Exception:
            return False

    def ports_listening(self, port_checker: PortChecker) -> bool:
        try:
            return bool(port_checker.are_ports_listening(self.ports.as_list()))
        except Exception:
            return False

    def get_status(self, port_checker: PortChecker) -> ServerStatus:
        running = self.proc_running()
        listening = self.ports_listening(port_checker)

        if running and listening:
            return ServerStatus.ONLINE
        if running and not listening:
            return ServerStatus.STARTING
        if (not running) and listening:
            return ServerStatus.ERROR
        return ServerStatus.OFFLINE

    def uptime(self) -> str:
        if not self.proc_running():
            return "--:--:--"
        elapsed = max(0, int(time.time() - self.started_at))
        h = elapsed // 3600
        m = (elapsed % 3600) // 60
        s = elapsed % 60
        return f"{h:02d}:{m:02d}:{s:02d}"
