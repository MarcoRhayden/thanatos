from __future__ import annotations

from ..services.orchestrator import ThanatosOrchestrator


def execute(orchestrator: ThanatosOrchestrator, n: int):
    return orchestrator.start_n_blocks(n)
