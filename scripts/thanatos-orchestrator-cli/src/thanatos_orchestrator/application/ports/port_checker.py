from __future__ import annotations

from typing import Protocol


class PortChecker(Protocol):
    """Port checking abstraction."""

    def is_port_listening(self, port: int) -> bool:
        ...

    def are_ports_listening(self, ports: list[int]) -> bool:
        ...
