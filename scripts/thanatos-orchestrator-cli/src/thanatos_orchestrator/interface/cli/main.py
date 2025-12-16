from __future__ import annotations

import atexit
from dataclasses import dataclass
from pathlib import Path

from thanatos_orchestrator.application.services.orchestrator import (
    OrchestratorPaths,
    ThanatosOrchestrator,
)
from thanatos_orchestrator.infrastructure.config.toml_config_repo import TomlConfigRepo
from thanatos_orchestrator.infrastructure.history.readline_history import ReadlineHistoryStore
from thanatos_orchestrator.infrastructure.ports.ss_port_checker import SsPortChecker
from thanatos_orchestrator.infrastructure.process.subprocess_runner import SubprocessRunner
from thanatos_orchestrator.infrastructure.system.app_settings_provider import AppSettingsProvider
from thanatos_orchestrator.infrastructure.system.signal_handlers import install_signal_handlers
from thanatos_orchestrator.interface.cli.command_router import CommandRouter
from thanatos_orchestrator.interface.cli.rich_ui import RichUI


@dataclass(frozen=True, slots=True)
class SystemClock:
    """Production clock."""

    def sleep(self, seconds: float) -> None:
        import time

        time.sleep(seconds)

    def time(self) -> float:
        import time

        return time.time()


def _infer_repo_root(config_path: Path, bin_path: Path) -> Path:
    """
    Infer a reasonable repository root from the provided config and binary paths.

    Primary heuristic:
      - If config is <root>/config/thanatos.toml, root is config_path.parents[1]
    Fallback:
      - Use the nearest common parent (best effort)
      - If that fails, use current working directory
    """
    cfg = config_path.resolve()
    bn = bin_path.resolve()

    # Heuristic: <root>/config/thanatos.toml
    if cfg.name == "thanatos.toml" and cfg.parent.name == "config":
        return cfg.parent.parent

    try:
        common = Path(*Path(Path.cwd()).parts)
        cfg_parts = cfg.parts
        bn_parts = bn.parts
        min_len = min(len(cfg_parts), len(bn_parts))
        common_parts = []
        for i in range(min_len):
            if cfg_parts[i] == bn_parts[i]:
                common_parts.append(cfg_parts[i])
            else:
                break
        if common_parts:
            return Path(*common_parts)
    except Exception:
        pass

    return Path.cwd().resolve()


def main() -> None:
    """
    Public CLI entrypoint for the package.

    This is called by `python -m thanatos_orchestrator`.
    It resolves CLI/ENV settings and then delegates to `run()`.
    """
    settings = AppSettingsProvider.from_env_and_cli()
    AppSettingsProvider.export_to_env(settings)

    run(
        config_path=Path(settings.config_path),
        bin_path=Path(settings.thanatos_bin),
        delay_ms=int(settings.delay_ms),
    )


def run(config_path: Path, bin_path: Path, delay_ms: int = 250) -> None:
    """
    Runs the interactive CLI loop.

    Notes:
    - config_path and bin_path are resolved via CLI/ENV by AppSettingsProvider.
    - delay_ms is exported to ENV as well (THANATOS_DELAY_MS) for other components.
    """
    base_config = config_path.resolve()
    server_bin = bin_path.resolve()

    repo_root = _infer_repo_root(base_config, server_bin)
    runtime_dir = (repo_root / ".thanatos-runtime").resolve()

    paths = OrchestratorPaths(
        repo_root=repo_root,
        base_config=base_config,
        server_bin=server_bin,
        runtime_dir=runtime_dir,
        cwd=repo_root,
    )

    orchestrator = ThanatosOrchestrator(
        paths=paths,
        config_repo=TomlConfigRepo(),
        process_runner=SubprocessRunner(),
        port_checker=SsPortChecker(),
        clock=SystemClock(),
    )

    history = ReadlineHistoryStore()
    history.setup(runtime_dir / "command_history.txt", max_len=5000)

    atexit.register(lambda: orchestrator.stop_all(reason="atexit"))
    install_signal_handlers(orchestrator.stop_all)

    RichUI.print_banner()
    router = CommandRouter(orchestrator)

    import traceback

    if delay_ms < 0:
        delay_ms = 0

    while True:
        try:
            raw = input("\033[1;36mthanatos> \033[0m")
        except (EOFError, KeyboardInterrupt):
            orchestrator.stop_all(reason="interactive exit")
            RichUI.print_message("Exiting...", "warning")
            break

        argv = raw.strip().split()
        if not argv:
            continue

        try:
            result = router.dispatch(argv)
            if result.should_exit:
                RichUI.print_message("Exiting...", "warning")
                break
        except ValueError as e:
            RichUI.print_message(f"Invalid argument: {e}", "error")
        except Exception as e:
            RichUI.print_message(f"Unexpected error: {e}", "error")
            traceback.print_exc()
