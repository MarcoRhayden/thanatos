from __future__ import annotations

from enum import Enum


class ServerStatus(Enum):
    """High-level status for a server instance.

    This enum intentionally carries presentation metadata (label/style/icon)
    to keep the CLI output stable and consistent across layers.
    """

    ONLINE = ("ACTIVE", "bold bright_green", "✦")
    STARTING = ("STARTING", "bold bright_yellow", "◈")
    STOPPING = ("STOPPING", "bold bright_yellow", "◇")
    OFFLINE = ("OFFLINE", "bold bright_red", "✗")
    ERROR = ("ERROR", "bold bright_red", "⚠")

    def __init__(self, label: str, style: str, icon: str) -> None:
        self.label = label
        self.style = style
        self.icon = icon
