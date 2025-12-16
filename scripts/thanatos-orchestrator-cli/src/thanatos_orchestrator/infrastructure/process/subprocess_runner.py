from __future__ import annotations

import os
import shutil
import subprocess
from pathlib import Path
from typing import Optional


class SubprocessRunner:
    """
    Launches Thanatos processes.

    Resolution order:
      1) thanatos_bin passed in constructor
      2) ENV: THANATOS_BIN
      3) default: ./Thanatos (or 'Thanatos' from PATH)
    """

    def terminate_process_group(self, proc, timeout: float) -> None:
        """
        Terminates a process group gracefully, sending SIGTERM and then SIGKILL if needed.
        """
        import os
        import signal
        import time
        if proc is None or proc.poll() is not None:
            return
        try:
            os.killpg(proc.pid, signal.SIGTERM)
        except (ProcessLookupError, Exception):
            try:
                proc.terminate()
            except Exception:
                return
        deadline = time.time() + timeout
        while time.time() < deadline:
            if proc.poll() is not None:
                return
            time.sleep(0.05)
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except (ProcessLookupError, Exception):
            try:
                proc.kill()
            except Exception:
                pass

    ENV_BIN = "THANATOS_BIN"

    def __init__(self, thanatos_bin: Optional[Path] = None) -> None:
        self._thanatos_bin = thanatos_bin or self._resolve_default_bin()

    def _resolve_default_bin(self) -> Path:
        env = os.environ.get(self.ENV_BIN)
        if env:
            return Path(env).expanduser().resolve()

        default_bin = (Path.cwd() / "Thanatos").resolve()
        if default_bin.exists():
            return default_bin

        which = shutil.which("Thanatos")
        if which:
            return Path(which).resolve()

        return default_bin

    @property
    def thanatos_bin(self) -> Path:
        return self._thanatos_bin

    def start(self, server_bin: Path, config_path: Path, cwd: Path) -> subprocess.Popen:
        cmd = [str(server_bin), "--config", str(config_path)]
        return subprocess.Popen(
            cmd,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            cwd=str(cwd),
            start_new_session=True,
        )
