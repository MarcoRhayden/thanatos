from __future__ import annotations

from ..services.orchestrator import ThanatosOrchestrator


def execute(orchestrator: ThanatosOrchestrator):
    return orchestrator.list_blocks(), orchestrator.get_counts()
