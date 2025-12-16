from __future__ import annotations

from dataclasses import dataclass
from typing import List

from ...application.services.orchestrator import ThanatosOrchestrator
from ...application.use_cases import (
    list_servers,
    start_server,
    stop_server,
    start_block,
    stop_block,
    start_n_blocks,
    stop_all,
    clear_screen,
)
from .rich_ui import RichUI


@dataclass(slots=True)
class CommandResult:
    should_exit: bool = False


class CommandRouter:
    """Parse and dispatch CLI commands."""

    def __init__(self, orchestrator: ThanatosOrchestrator) -> None:
        self.orchestrator = orchestrator

    def dispatch(self, argv: List[str]) -> CommandResult:
        if not argv:
            return CommandResult(False)

        command = argv[0].strip()

        if command == "help":
            RichUI.print_help()
            return CommandResult(False)

        if command == "clear":
            result = clear_screen.execute()
            RichUI.clear_screen()
            if result.should_redraw_banner:
                RichUI.print_banner()
            return CommandResult(False)

        if command == "list":
            blocks, (online, total) = list_servers.execute(self.orchestrator)
            if total == 0:
                RichUI.print_message("No Thanatos servers are currently running.", "warning")
                return CommandResult(False)
            from rich.progress import Progress, SpinnerColumn, TextColumn
            with Progress(
                SpinnerColumn(),
                TextColumn("[progress.description]{task.description}"),
                transient=True,
            ) as progress:
                task = progress.add_task("Checking servers...", total=None)
                table = RichUI.create_server_table(blocks, online, total, self.orchestrator.port_checker)
                progress.update(task, description="Done!")
            if table.row_count > 0:
                from rich.console import Console
                Console().print(table)
            return CommandResult(False)

        if command == "start-server":
            inst = start_server.execute(self.orchestrator)
            RichUI.print_message(
                f"Started block={inst.block} slot={inst.slot} | "
                f"login={inst.ports.login} char={inst.ports.char} query={inst.ports.query} | "
                f"name={inst.server_name}",
                "success",
            )
            return CommandResult(False)

        if command == "stop-server":
            if len(argv) != 3:
                RichUI.print_message("Usage: stop-server <block> <slot>", "error")
                return CommandResult(False)

            block = int(argv[1])
            slot = int(argv[2])
            stop_server.execute(self.orchestrator, block, slot)
            RichUI.print_message(f"Stopped block={block} slot={slot}", "warning")
            return CommandResult(False)

        if command == "start-block":
            started = start_block.execute(self.orchestrator)
            for inst in started:
                RichUI.print_message(
                    f"Started block={inst.block} slot={inst.slot} | "
                    f"login={inst.ports.login} char={inst.ports.char} query={inst.ports.query} | "
                    f"name={inst.server_name}",
                    "success",
                )
            return CommandResult(False)

        if command == "stop-block":
            if len(argv) != 2:
                RichUI.print_message("Usage: stop-block <block>", "error")
                return CommandResult(False)

            block = int(argv[1])
            stop_block.execute(self.orchestrator, block)
            RichUI.print_message(f"Stopped block={block}", "warning")
            return CommandResult(False)

        if command == "start-n-blocks":
            if len(argv) != 2:
                RichUI.print_message("Usage: start-n-blocks <n>", "error")
                return CommandResult(False)
            n = int(argv[1])
            from rich.progress import Progress, SpinnerColumn, TextColumn
            with Progress(
                SpinnerColumn(),
                TextColumn("[progress.description]{task.description}"),
                transient=True,
            ) as progress:
                task = progress.add_task(f"Starting {n} block(s)...", total=None)
                started = start_n_blocks.execute(self.orchestrator, n)
                progress.update(task, description=f"Finished starting {len(started)} server(s)")
            RichUI.print_message(f"Started {len(started)} server(s) across {n} block(s)", "success")
            return CommandResult(False)

        if command == "exit":
            stop_all.execute(self.orchestrator, reason="exit command")
            return CommandResult(True)

        RichUI.print_message(f"Unknown command: '{command}'. Type 'help' for available commands.", "error")
        return CommandResult(False)
