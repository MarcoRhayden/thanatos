from __future__ import annotations

from ..services.orchestrator import ThanatosOrchestrator


def execute(orchestrator: ThanatosOrchestrator, reason: str = "shutdown") -> None:
    orchestrator.stop_all(reason=reason)
