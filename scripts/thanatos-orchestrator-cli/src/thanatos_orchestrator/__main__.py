from __future__ import annotations

import sys

from .infrastructure.system.app_settings_provider import AppSettingsProvider


def main(argv: list[str] | None = None) -> None:
    """
    Entry point: python -m thanatos_orchestrator

    Responsibilities:
      - Parse CLI args / ENV
      - Validate paths
      - Export settings to ENV for compatibility
      - Delegate to CLI loop
    """
    settings = AppSettingsProvider.from_env_and_cli(argv)
    AppSettingsProvider.export_to_env(settings)

    # Import the CLI only after settings are resolved to avoid import-time path issues.
    from .interface.cli.main import main as cli_main

    cli_main()


if __name__ == "__main__":
    main(sys.argv[1:])
