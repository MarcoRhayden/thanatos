from __future__ import annotations

import os
from dataclasses import dataclass
from pathlib import Path
from typing import Optional, Set, Any, List, Tuple

from ...domain.entities.port_triple import PortTriple
from ...domain.entities.server_instance import ServerInstance
from ...domain.enums.server_status import ServerStatus

from ..ports.clock import Clock
from ..ports.port_checker import PortChecker
from ..ports.process_runner import ProcessRunner, RunningProcess
from ..ports.config_repo import ConfigRepo


@dataclass(frozen=True, slots=True)
class OrchestratorPaths:
    """Resolved paths required by the orchestrator."""

    repo_root: Path
    base_config: Path
    server_bin: Path
    runtime_dir: Path
    cwd: Path


class ThanatosOrchestrator:
    """Application service that manages Thanatos server blocks."""

    def __init__(
        self,
        paths: OrchestratorPaths,
        config_repo: ConfigRepo,
        process_runner: ProcessRunner,
        port_checker: PortChecker,
        clock: Clock,
        block_size: int = 3,
        start_delay_seconds: float = 0.35,
        startup_wait_seconds: float = 2.0,
        startup_poll_interval: float = 0.10,
        shutdown_term_timeout: float = 2.0,
    ) -> None:
        self.paths = paths
        self.config_repo = config_repo
        self.process_runner = process_runner
        self.port_checker = port_checker
        self.clock = clock

        self.block_size = int(block_size)
        self.start_delay_seconds = float(start_delay_seconds)
        self.startup_wait_seconds = float(startup_wait_seconds)
        self.startup_poll_interval = float(startup_poll_interval)
        self.shutdown_term_timeout = float(shutdown_term_timeout)

        self.blocks: List[List[Optional[ServerInstance]]] = []
        self._reserved_ports: Set[int] = set()
        self._shutdown_in_progress = False

        self._base_cfg: Optional[dict[str, Any]] = None
        self._port_triples: Optional[list[PortTriple]] = None

        self._validate_environment()
        self._ensure_runtime_dir()

    def _validate_environment(self) -> None:
        if not self.paths.base_config.exists():
            raise SystemExit(f"Configuration not found: {self.paths.base_config}")
        if not self.paths.server_bin.exists():
            raise SystemExit(f"Server binary not found: {self.paths.server_bin}")
        if not os.access(str(self.paths.server_bin), os.X_OK):
            raise SystemExit(f"Server binary not executable: {self.paths.server_bin}")

    def _ensure_runtime_dir(self) -> None:
        self.paths.runtime_dir.mkdir(parents=True, exist_ok=True)

    def _load_base(self) -> dict[str, Any]:
        if self._base_cfg is None:
            self._base_cfg = self.config_repo.load_base_config(self.paths.base_config)
        return self._base_cfg

    def _load_port_triples(self) -> list[PortTriple]:
        if self._port_triples is None:
            self._port_triples = self.config_repo.extract_port_triples(self.paths.base_config)
        return self._port_triples

    def _find_first_block_with_free_slot(self) -> Optional[int]:
        for bidx, block in enumerate(self.blocks):
            if any(slot is None for slot in block):
                return bidx
        return None

    def _ensure_block(self, block_idx: int) -> None:
        while len(self.blocks) <= block_idx:
            self.blocks.append([None] * self.block_size)

    def _allocate_slot(self, preferred_block: Optional[int] = None) -> Tuple[int, int]:
        if preferred_block is not None:
            if preferred_block < 0:
                raise ValueError("Block index must be >= 0")
            self._ensure_block(preferred_block)
            for slot in range(self.block_size):
                if self.blocks[preferred_block][slot] is None:
                    return preferred_block, slot
            raise ValueError(f"Block {preferred_block} is full")

        bidx = self._find_first_block_with_free_slot()
        if bidx is not None:
            for slot in range(self.block_size):
                if self.blocks[bidx][slot] is None:
                    return bidx, slot

        self.blocks.append([None] * self.block_size)
        return len(self.blocks) - 1, 0

    def _recompute_reserved_ports(self) -> None:
        reserved: Set[int] = set()
        for block in self.blocks:
            for inst in block:
                if inst is None:
                    continue
                if inst.proc_running() or inst.ports_listening(self.port_checker):
                    reserved.update(inst.ports.as_list())
        self._reserved_ports = reserved

    def _pick_first_available_triple(self, extra_reserved: Optional[Set[int]] = None) -> PortTriple:
        self._recompute_reserved_ports()
        reserved = set(self._reserved_ports)
        if extra_reserved:
            reserved |= set(extra_reserved)

        triples = self._load_port_triples()
        for t in triples:
            ports = t.as_list()
            if any(p in reserved for p in ports):
                continue
            if all(not self.port_checker.is_port_listening(p) for p in ports):
                return t

        raise RuntimeError("No available port triples found (all in use)")

    def _wait_until_listening_or_exit(self, proc: RunningProcess, ports: list[int]) -> tuple[bool, Optional[int]]:
        deadline = self.clock.time() + self.startup_wait_seconds
        while self.clock.time() < deadline:
            rc = proc.poll()
            if rc is not None:
                return False, rc
            if self.port_checker.are_ports_listening(ports):
                return True, None
            self.clock.sleep(self.startup_poll_interval)

        rc = proc.poll()
        return self.port_checker.are_ports_listening(ports), rc

    def get_counts(self) -> tuple[int, int]:
        total = 0
        online = 0
        for block in self.blocks:
            for inst in block:
                if inst is None:
                    continue
                total += 1
                if inst.get_status(self.port_checker) == ServerStatus.ONLINE:
                    online += 1
        return online, total

    def list_blocks(self) -> List[List[Optional[ServerInstance]]]:
        self._recompute_reserved_ports()
        return self.blocks

    def start_server(self, preferred_block: Optional[int] = None, extra_reserved: Optional[Set[int]] = None) -> ServerInstance:
        base_cfg = self._load_base()
        ports = self._pick_first_available_triple(extra_reserved=extra_reserved)

        block, slot = self._allocate_slot(preferred_block=preferred_block)

        if extra_reserved is not None:
            extra_reserved.update(ports.as_list())
        self._reserved_ports.update(ports.as_list())

        server_name = f"Thanatos-b{block}-s{slot}-i{ports.config_index}"

        instance_dir = self.paths.runtime_dir / f"block{block}" / f"srv{slot}"
        instance_dir.mkdir(parents=True, exist_ok=True)

        config_path = instance_dir / "thanatos.toml"
        self.config_repo.write_instance_config(
            dest=config_path,
            base_cfg=base_cfg,
            ports=ports,
            server_name=server_name,
            instance_dir=instance_dir,
        )

        proc = self.process_runner.start(self.paths.server_bin, config_path, self.paths.cwd)

        inst = ServerInstance(
            block=block,
            slot=slot,
            proc=proc,
            ports=ports,
            server_name=server_name,
            config_path=config_path,
        )
        self.blocks[block][slot] = inst

        self._wait_until_listening_or_exit(proc, ports.as_list())
        self.clock.sleep(self.start_delay_seconds)
        return inst

    def stop_server(self, block_idx: int, slot_idx: int) -> None:
        if block_idx < 0 or block_idx >= len(self.blocks):
            raise ValueError("Block not found")
        if slot_idx < 0 or slot_idx >= self.block_size:
            raise ValueError("Invalid slot index (expected 0..2)")

        inst = self.blocks[block_idx][slot_idx]
        if inst is None:
            raise ValueError("Server not found (empty slot)")

        self.process_runner.terminate_process_group(inst.proc, timeout=self.shutdown_term_timeout)
        self.blocks[block_idx][slot_idx] = None
        self._recompute_reserved_ports()

    def start_block(self) -> list[ServerInstance]:
        preferred = self._find_first_block_with_free_slot()
        if preferred is None:
            self.blocks.append([None] * self.block_size)
            preferred = len(self.blocks) - 1

        batch_reserved: Set[int] = set()
        started: list[ServerInstance] = []

        while True:
            if all(self.blocks[preferred][s] is not None for s in range(self.block_size)):
                break
            inst = self.start_server(preferred_block=preferred, extra_reserved=batch_reserved)
            started.append(inst)

        return started

    def stop_block(self, block_idx: int) -> None:
        if block_idx < 0 or block_idx >= len(self.blocks):
            raise ValueError("Block not found")

        for slot in range(self.block_size):
            if self.blocks[block_idx][slot] is not None:
                self.stop_server(block_idx, slot)

    def start_n_blocks(self, n: int) -> list[ServerInstance]:
        started: list[ServerInstance] = []
        blocks_possible = self._count_possible_blocks()
        if n > blocks_possible:
            from thanatos_orchestrator.interface.cli.rich_ui import RichUI
            RichUI.print_message(
                f"Only {blocks_possible} block(s) can be started with the currently available ports.",
                "error",
            )
            n = blocks_possible
        for _ in range(int(n)):
            try:
                started.extend(self.start_block())
            except RuntimeError as e:
                from thanatos_orchestrator.interface.cli.rich_ui import RichUI
                RichUI.print_message(str(e), "error")
                break
        return started

    def _count_possible_blocks(self) -> int:
        # Calculates how many full blocks can be created with the available ports
        triples = self._load_port_triples()
        self._recompute_reserved_ports()
        reserved = set(self._reserved_ports)
        free_triples = [t for t in triples if all(p not in reserved for p in t.as_list())]
        return len(free_triples) // self.block_size

    def stop_all(self, reason: str = "shutdown") -> None:
        if self._shutdown_in_progress:
            return
        self._shutdown_in_progress = True

        # Collect all alive processes
        procs = []
        for block in self.blocks:
            for inst in block:
                if inst and inst.proc and inst.proc.poll() is None:
                    procs.append(inst.proc)

        if not procs:
            self._recompute_reserved_ports()
            return

        # Send SIGTERM to all
        for p in procs:
            try:
                import os, signal
                os.killpg(p.pid, signal.SIGTERM)
            except Exception:
                try:
                    p.terminate()
                except Exception:
                    pass

        # Wait until timeout
        import time
        deadline = time.time() + self.shutdown_term_timeout
        while time.time() < deadline:
            alive = [p for p in procs if p.poll() is None]
            if not alive:
                break
            time.sleep(0.05)

        # Send SIGKILL to remaining
        for p in procs:
            if p.poll() is None:
                try:
                    import os, signal
                    os.killpg(p.pid, signal.SIGKILL)
                except Exception:
                    try:
                        p.kill()
                    except Exception:
                        pass

        # Clear all slots
        for bidx in range(len(self.blocks)):
            for sidx in range(self.block_size):
                self.blocks[bidx][sidx] = None

        self._recompute_reserved_ports()
