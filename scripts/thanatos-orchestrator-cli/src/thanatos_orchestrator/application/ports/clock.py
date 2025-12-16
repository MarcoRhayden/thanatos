from __future__ import annotations

from typing import Protocol


class Clock(Protocol):
    """Clock abstraction to improve testability."""

    def sleep(self, seconds: float) -> None:
        ...

    def time(self) -> float:
        ...
