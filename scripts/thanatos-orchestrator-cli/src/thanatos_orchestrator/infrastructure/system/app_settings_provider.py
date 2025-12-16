from __future__ import annotations

import argparse
import os
import shutil
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

from ...application.ports.app_settings import AppSettings


@dataclass(frozen=True)
class ResolvedPaths:
    config_path: Path
    thanatos_bin: Path


class AppSettingsProvider:
    """
    Resolves bootstrap settings from CLI and ENV with precedence:

      CLI > ENV > defaults

    ENV variables:
      - THANATOS_CONFIG
      - THANATOS_BIN
      - THANATOS_DELAY_MS
    """

    ENV_CONFIG = "THANATOS_CONFIG"
    ENV_BIN = "THANATOS_BIN"
    ENV_DELAY = "THANATOS_DELAY_MS"

    @staticmethod
    def build_arg_parser() -> argparse.ArgumentParser:
        parser = argparse.ArgumentParser(
            prog="thanatos-orchestrator",
            add_help=True,
            description="Thanatos multi-server orchestrator (CLI + ENV bootstrap).",
        )
        parser.add_argument(
            "--config",
            dest="config",
            type=str,
            default=None,
            help="Path to Thanatos base config TOML (overrides THANATOS_CONFIG).",
        )
        parser.add_argument(
            "--bin",
            dest="binary",
            type=str,
            default=None,
            help="Path to Thanatos binary (overrides THANATOS_BIN).",
        )
        parser.add_argument(
            "--delay-ms",
            dest="delay_ms",
            type=int,
            default=None,
            help="Delay between server starts in milliseconds (overrides THANATOS_DELAY_MS).",
        )
        parser.add_argument(
            "--no-validate",
            dest="no_validate",
            action="store_true",
            help="Disable filesystem validation for config and binary paths.",
        )
        return parser

    @staticmethod
    def from_env_and_cli(argv: Optional[list[str]] = None) -> AppSettings:
        parser = AppSettingsProvider.build_arg_parser()
        args = parser.parse_args(argv)

        config_str = args.config or os.environ.get(AppSettingsProvider.ENV_CONFIG)
        bin_str = args.binary or os.environ.get(AppSettingsProvider.ENV_BIN)

        delay_env = os.environ.get(AppSettingsProvider.ENV_DELAY)
        delay_ms = args.delay_ms if args.delay_ms is not None else None
        if delay_ms is None and delay_env is not None:
            try:
                delay_ms = int(delay_env)
            except ValueError:
                raise SystemExit(
                    f"Invalid {AppSettingsProvider.ENV_DELAY} value: {delay_env!r}. Must be an integer."
                )

        if delay_ms is None:
            delay_ms = 250

        paths = AppSettingsProvider._resolve_paths(config_str=config_str, bin_str=bin_str)

        if not args.no_validate:
            AppSettingsProvider._validate(paths)

        return AppSettings(
            config_path=paths.config_path,
            thanatos_bin=paths.thanatos_bin,
            delay_ms=delay_ms,
        )

    @staticmethod
    def _resolve_paths(config_str: Optional[str], bin_str: Optional[str]) -> ResolvedPaths:
        # Defaults are relative to the current working directory (project root recommended).
        default_config = Path.cwd() / "config" / "thanatos.toml"
        default_bin = Path.cwd() / "Thanatos"

        config_path = Path(config_str).expanduser() if config_str else default_config
        config_path = config_path.resolve()

        if bin_str:
            thanatos_bin = Path(bin_str).expanduser().resolve()
        else:
            # If default bin does not exist, try resolving from PATH.
            if default_bin.exists():
                thanatos_bin = default_bin.resolve()
            else:
                which = shutil.which("Thanatos")
                if which:
                    thanatos_bin = Path(which).resolve()
                else:
                    thanatos_bin = default_bin.resolve()

        return ResolvedPaths(config_path=config_path, thanatos_bin=thanatos_bin)

    @staticmethod
    def _validate(paths: ResolvedPaths) -> None:
        if not paths.config_path.exists() or not paths.config_path.is_file():
            raise SystemExit(
                "Configuration not found: "
                f"{paths.config_path}\n"
                "Fix:\n"
                "  - pass --config /path/to/thanatos.toml\n"
                f"  - or set {AppSettingsProvider.ENV_CONFIG}=/path/to/thanatos.toml\n"
                "  - or place it at ./config/thanatos.toml (recommended default)"
            )

        if not paths.thanatos_bin.exists() or not paths.thanatos_bin.is_file():
            raise SystemExit(
                "Thanatos binary not found: "
                f"{paths.thanatos_bin}\n"
                "Fix:\n"
                "  - pass --bin /path/to/Thanatos\n"
                f"  - or set {AppSettingsProvider.ENV_BIN}=/path/to/Thanatos\n"
                "  - or ensure 'Thanatos' is available in PATH\n"
                "  - or place it at ./Thanatos (recommended default)"
            )

        # Executable check (best effort)
        if os.name != "nt" and not os.access(paths.thanatos_bin, os.X_OK):
            raise SystemExit(
                f"Thanatos binary is not executable: {paths.thanatos_bin}\n"
                "Fix:\n"
                f"  chmod +x {paths.thanatos_bin}"
            )

    @staticmethod
    def export_to_env(settings: AppSettings) -> None:
        """
        Exports resolved settings to ENV.

        This keeps the rest of the codebase compatible even if some components
        still read settings from environment variables.
        """
        os.environ[AppSettingsProvider.ENV_CONFIG] = str(settings.config_path)
        os.environ[AppSettingsProvider.ENV_BIN] = str(settings.thanatos_bin)
        os.environ[AppSettingsProvider.ENV_DELAY] = str(settings.delay_ms)
