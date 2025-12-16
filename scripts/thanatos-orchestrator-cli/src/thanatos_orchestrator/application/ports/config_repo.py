from __future__ import annotations

from pathlib import Path
from typing import Any, Protocol


class ConfigRepo(Protocol):
    """
    Application port: reads the base Thanatos TOML configuration.
    Infrastructure adapters must implement this interface.
    """

    def load_base_config(self, path: Path) -> dict[str, Any]:
        """Load and return the parsed TOML as a dict."""
        raise NotImplementedError
