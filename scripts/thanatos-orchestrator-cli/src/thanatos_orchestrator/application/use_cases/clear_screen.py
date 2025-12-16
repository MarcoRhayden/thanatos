from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True, slots=True)
class ClearScreenResult:
    """Result returned by the clear-screen use-case."""
    should_redraw_banner: bool = True


def execute() -> ClearScreenResult:
    """Use-case: clear the terminal screen and request banner redraw.

    This is intentionally UI-agnostic: it does not perform any I/O.
    The interface layer decides how to clear/redraw.
    """
    return ClearScreenResult(should_redraw_banner=True)
