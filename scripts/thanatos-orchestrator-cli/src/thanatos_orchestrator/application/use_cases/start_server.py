from __future__ import annotations

from ..services.orchestrator import ThanatosOrchestrator
from ...domain.entities.server_instance import ServerInstance


def execute(orchestrator: ThanatosOrchestrator) -> ServerInstance:
    return orchestrator.start_server()
