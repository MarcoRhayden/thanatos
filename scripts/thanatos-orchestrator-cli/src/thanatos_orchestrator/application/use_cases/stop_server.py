from __future__ import annotations

from ..services.orchestrator import ThanatosOrchestrator


def execute(orchestrator: ThanatosOrchestrator, block: int, slot: int) -> None:
    orchestrator.stop_server(block, slot)
