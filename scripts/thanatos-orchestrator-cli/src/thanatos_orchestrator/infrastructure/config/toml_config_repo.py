from __future__ import annotations

from pathlib import Path
from typing import Any

import toml

from thanatos_orchestrator.application.ports.config_repo import ConfigRepo


class TomlConfigRepo(ConfigRepo):


    def write_instance_config(
        self,
        dest: Path,
        base_cfg: dict,
        ports,
        server_name: str,
        instance_dir: Path,
    ) -> None:
        import toml
        import copy
        # Deep copy base_cfg to avoid mutating the original
        cfg = toml.loads(toml.dumps(base_cfg))
        cfg.setdefault("thanatos", {})
        cfg.setdefault("query", {})

        # Update ports
        def set_port_field(obj, key, port):
            cur = obj.get(key, None)
            obj[key] = [int(port)] if isinstance(cur, list) else int(port)

        set_port_field(cfg["thanatos"], "login_ports", ports.login)
        set_port_field(cfg["thanatos"], "char_ports", ports.char)
        cfg["thanatos"]["server_name"] = server_name
        set_port_field(cfg["query"], "ports", ports.query)

        # Patch singleton paths to the instance directory
        singleton_keys = {
            "pidfile", "pid_file", "pidpath", "pid_path",
            "lockfile", "lock_file",
            "socket", "socket_path", "unix_socket", "unixsocket",
            "state_file", "statefile", "state_path",
            "control_socket", "controlsocket",
        }
        def walk(obj):
            if isinstance(obj, dict):
                for k, v in obj.items():
                    if k in singleton_keys and isinstance(v, str):
                        obj[k] = str(instance_dir / Path(v).name)
                    else:
                        walk(v)
            elif isinstance(obj, list):
                for v in obj:
                    walk(v)
        walk(cfg)

        dest = Path(dest).expanduser().resolve()
        dest.parent.mkdir(parents=True, exist_ok=True)
        with open(dest, "w", encoding="utf-8") as f:
            toml.dump(cfg, f)

    def extract_port_triples(self, path: Path):
        from thanatos_orchestrator.domain.entities.port_triple import PortTriple
        data = self.load_base_config(path)
        login_ports = data.get('thanatos', {}).get('login_ports', [])
        char_ports = data.get('thanatos', {}).get('char_ports', [])
        query_ports = data.get('query', {}).get('ports', [])
        n = min(len(login_ports), len(char_ports), len(query_ports))
        triples = []
        for i in range(n):
            triples.append(PortTriple(
                config_index=i,
                login=login_ports[i],
                char=char_ports[i],
                query=query_ports[i],
            ))
        return triples

    def load_base_config(self, path: Path) -> dict[str, Any]:
        """
        Load a TOML file from disk and return it as a dict.

        Expected sections used by the orchestrator:
          - [thanatos].login_ports (list[int])
          - [thanatos].char_ports  (list[int])
          - [query].ports          (list[int])
        """
        p = Path(path).expanduser().resolve()

        if not p.exists():
            raise FileNotFoundError(f"Base configuration not found: {p}")

        if not p.is_file():
            raise IsADirectoryError(f"Base configuration must be a file, got directory: {p}")

        data = toml.load(str(p))
        if not isinstance(data, dict):
            raise ValueError("Invalid TOML format: expected a dict at the top level.")

        return data

    def load(self, path: Path) -> dict[str, Any]:
        return self.load_base_config(path)

    def read(self, path: Path) -> dict[str, Any]:
        return self.load_base_config(path)
