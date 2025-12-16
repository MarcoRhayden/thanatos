from __future__ import annotations

import socket
import subprocess

from ...application.ports.port_checker import PortChecker


class SsPortChecker(PortChecker):
    """Checks listening ports using `ss`, with a TCP fallback."""

    def __init__(self) -> None:
        self._ss_available = self._detect_ss()

    def _detect_ss(self) -> bool:
        try:
            subprocess.run(["ss", "-h"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
            return True
        except FileNotFoundError:
            return False

    def is_port_listening(self, port: int) -> bool:
        port = int(port)

        if self._ss_available:
            tcp = subprocess.run(
                ["ss", "-lntH", f"sport = :{port}"],
                stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL,
                text=True,
                check=False,
            )
            if tcp.stdout.strip():
                return True

            udp = subprocess.run(
                ["ss", "-lunH", f"sport = :{port}"],
                stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL,
                text=True,
                check=False,
            )
            return bool(udp.stdout.strip())

        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.25):
                return True
        except OSError:
            return False

    def are_ports_listening(self, ports: list[int]) -> bool:
        return all(self.is_port_listening(int(p)) for p in ports)
