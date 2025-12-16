from __future__ import annotations

from typing import List, Optional

from rich.align import Align
from rich.box import DOUBLE
from rich.console import Console
from rich.panel import Panel
from rich.table import Table
from rich.text import Text

from ...application.ports.port_checker import PortChecker
from ...domain.entities.server_instance import ServerInstance

console = Console()


class FFColors:
    """A simple color palette used by the CLI."""

    CRYSTAL_BLUE = "bright_cyan"
    MANA_PURPLE = "bright_magenta"
    PHOENIX_RED = "bright_red"
    CHOCOBO_YELLOW = "bright_yellow"
    LIFE_GREEN = "bright_green"
    GOLD = "yellow"


class RichUI:
    """Rich-based CLI UI."""

    CONTACT_LINE = "rhayden@arkansoftware.com"

    # Column widths (keep these stable to avoid drift across terminals)
    _W_BLOCK = 5
    _W_SLOT = 4
    _W_PID = 8
    _W_STATUS = 18
    _W_LOGIN = 6
    _W_CHAR = 5
    _W_QUERY = 7
    _W_UPTIME = 10

    # Use ASCII-only status icon to avoid wcwidth ambiguity in some fonts/terminals.
    _STATUS_ICON = "*"

    @staticmethod
    def clear_screen() -> None:
        console.clear()

    @staticmethod
    def print_banner() -> None:
        title = Text()
        title.append("THANATOS", style=f"bold {FFColors.MANA_PURPLE}")
        title.append("  ")
        title.append("Multi-Server Orchestrator", style=FFColors.CRYSTAL_BLUE)

        body = Text()
        body.append("Type ", style=FFColors.CHOCOBO_YELLOW)
        body.append("help", style=f"bold {FFColors.GOLD}")
        body.append(" to show commands. ", style=FFColors.CHOCOBO_YELLOW)
        body.append("Type ", style=FFColors.CHOCOBO_YELLOW)
        body.append("list", style=f"bold {FFColors.GOLD}")
        body.append(" to inspect blocks.\n", style=FFColors.CHOCOBO_YELLOW)
        body.append(RichUI.CONTACT_LINE, style=FFColors.CRYSTAL_BLUE)

        panel = Panel(
            Align.left(Text.assemble(title, "\n", body)),
            border_style=FFColors.MANA_PURPLE,
            expand=False,
        )
        console.print(panel)

    @staticmethod
    def print_help() -> None:
        lines = Text()
        lines.append("Usage:\n", style=f"bold {FFColors.CRYSTAL_BLUE}")
        lines.append("  thanatos> <command> [args]\n\n", style=FFColors.CHOCOBO_YELLOW)

        lines.append("Command Summary:\n", style=f"bold {FFColors.MANA_PURPLE}")

        commands = [
            ("list", "Display all tracked servers and verify whether ports are listening"),
            ("start-server", "Start a new server instance"),
            ("stop-server <block> <slot>", "Stop a specific server instance"),
            ("start-block", "Fill the earliest block up to 3 servers (creates new block only if needed)"),
            ("stop-block <block>", "Stop an entire server block"),
            ("start-n-blocks <n>", "Start multiple server blocks (fills partial blocks first)"),
            ("clear", "Clear the terminal and redisplay the banner"),
            ("help", "Display this help message"),
            ("exit", "Exit the manager (stops all servers first)"),
        ]

        for cmd, desc in commands:
            lines.append(f"  {cmd:<28} ", style=f"bold {FFColors.GOLD}")
            lines.append(f"{desc}\n", style=FFColors.CRYSTAL_BLUE)

        console.print(Panel(lines, border_style=FFColors.CRYSTAL_BLUE, title="help", expand=False))

    @staticmethod
    def print_message(message: str, kind: str = "info") -> None:
        kind = kind.lower().strip()
        if kind == "success":
            style = f"bold {FFColors.LIFE_GREEN}"
        elif kind == "error":
            style = f"bold {FFColors.PHOENIX_RED}"
        elif kind == "warning":
            style = f"bold {FFColors.CHOCOBO_YELLOW}"
        else:
            style = FFColors.CRYSTAL_BLUE

        console.print(message, style=style)

    @staticmethod
    def _status_cell(label: str, style: str) -> Text:
        label_width = max(0, RichUI._W_STATUS - 2)
        safe_label = (label or "").strip()

        if len(safe_label) > label_width:
            safe_label = safe_label[:label_width]
        else:
            safe_label = safe_label.ljust(label_width)

        t = Text(f"{RichUI._STATUS_ICON} {safe_label}")
        t.stylize(f"bold {style}" if "bold" not in style else style)
        return t

    @staticmethod
    def create_server_table(
        blocks: List[List[Optional[ServerInstance]]],
        online: int,
        total: int,
        port_checker: PortChecker,
    ) -> Table:
        title = Text()
        title.append("Thanatos Server Blocks (3 per block)", style=f"bold {FFColors.MANA_PURPLE}")
        title.append("  ")
        title.append("Online: ", style=FFColors.CHOCOBO_YELLOW)
        title.append(str(online), style=f"bold {FFColors.LIFE_GREEN}")
        title.append(" / ", style=FFColors.CHOCOBO_YELLOW)
        title.append(str(total), style=f"bold {FFColors.CRYSTAL_BLUE}")

        table = Table(
            title=title,
            show_lines=True,
            box=DOUBLE,
            border_style=FFColors.CRYSTAL_BLUE,
            title_style=f"bold {FFColors.MANA_PURPLE}",
            expand=True,
        )

        table.add_column("Block", style=f"bold {FFColors.MANA_PURPLE}", justify="center", width=RichUI._W_BLOCK, no_wrap=True)
        table.add_column("Slot", style=f"bold {FFColors.GOLD}", justify="center", width=RichUI._W_SLOT, no_wrap=True)
        table.add_column("PID", style=FFColors.CHOCOBO_YELLOW, justify="right", width=RichUI._W_PID, no_wrap=True)

        table.add_column("Status", justify="center", width=RichUI._W_STATUS, no_wrap=True)

        table.add_column("Login", style=FFColors.CRYSTAL_BLUE, justify="right", width=RichUI._W_LOGIN, no_wrap=True)
        table.add_column("Char", style=FFColors.CRYSTAL_BLUE, justify="right", width=RichUI._W_CHAR, no_wrap=True)
        table.add_column("Query", style=FFColors.CRYSTAL_BLUE, justify="right", width=RichUI._W_QUERY, no_wrap=True)

        table.add_column(
            "Name",
            justify="left",
            ratio=2,
            no_wrap=True,
            overflow="ellipsis",
        )

        table.add_column("Uptime", style=FFColors.CHOCOBO_YELLOW, justify="right", width=RichUI._W_UPTIME, no_wrap=True)

        any_rows = False

        for bidx, block in enumerate(blocks):
            for sidx, inst in enumerate(block):
                if inst is None:
                    continue

                any_rows = True

                status = inst.get_status(port_checker)
                status_text = RichUI._status_cell(status.label, status.style)

                listening = inst.ports_listening(port_checker)
                name_style = f"bold {FFColors.PHOENIX_RED}" if not listening else f"bold {FFColors.LIFE_GREEN}"
                name_text = Text(inst.server_name, style=name_style)

                table.add_row(
                    str(bidx),
                    str(sidx),
                    str(inst.proc.pid if inst.proc else "-"),
                    status_text,
                    str(inst.ports.login),
                    str(inst.ports.char),
                    str(inst.ports.query),
                    name_text,
                    inst.uptime(),
                )

        if not any_rows:
            RichUI.print_message("No servers are currently tracked.", "warning")

        return table
        