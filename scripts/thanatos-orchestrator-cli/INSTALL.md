# THANATOS - Multi-Server Orchestrator

A command-line tool to manage multiple Thanatos server instances with automatic ascending port allocation, process supervision, and live port listening verification.

---

## Requirements

- Python 3.10+ (recommended on modern distros)
- Linux (tested on Arch Linux)
- `ss` command available (package: `iproute2`)
- Thanatos server binary (`Thanatos`)
- A valid `thanatos.toml` base configuration

---

## Project Layout (important)

This project uses a **src-layout**:

- **Project root**: the directory that contains `pyproject.toml`
- **Package code**: located under `src/thanatos_orchestrator/`

Example:

```text
thanatos-orchestrator-cli/          <-- PROJECT ROOT (you run commands here)
  pyproject.toml
  .venv/
  INSTALL.md
  src/
    thanatos_orchestrator/          <-- Python package
      __main__.py
      ...
```
---

## Installation (Virtualenv)

### 1) Go to the project root

```bash
cd /path/to/thanatos-orchestrator-cli
# sanity check: you should see pyproject.toml
ls -la pyproject.toml
```

### 2) Create the virtual environment (one-time)

```bash
python -m venv .venv
```

If `.venv/` already exists, you can skip this step.

### 3) Activate the virtual environment (every new terminal)

```bash
source .venv/bin/activate
```

Sanity check (must point to .venv):

```bash
which python
python -c "import sys; print(sys.executable)"
```

### 4) Upgrade pip inside the venv

```bash
python -m pip install -U pip
```

### 5) Install dependencies

#### Recommended: install the project in editable mode

This is the best option for development: changes in `src/` take effect immediately.

```bash
python -m pip install -e .
```

> On Arch Linux, this is also the correct way to avoid the PEP 668 “externally-managed-environment” error.

#### Alternative: install only runtime deps (without editable install)

```bash
python -m pip install rich toml
```

---

## Configuration

You must point the orchestrator to:

1) the base `thanatos.toml` (contains `thanatos.login_ports`, `thanatos.char_ports`, `query.ports`)
2) the Thanatos binary path (`Thanatos`)

### How to locate where to set paths (project is modular)

From the project root, search for configuration defaults:

```bash
rg -n "BASE_CONFIG|SERVER_BIN|thanatos\.toml|Thanatos" src/thanatos_orchestrator
```

Update the values in the appropriate module so the orchestrator always reads the correct base config and calls the correct Thanatos binary.

---

## Usage

### Run the manager (recommended)

From the **project root**:

```bash
source .venv/bin/activate
python -m thanatos_orchestrator \
  --config /path/to/thanatos.toml \
  --bin /path/to/Thanatos \
  --delay-ms 250
```

Replace `/path/to/thanatos.toml` and `/path/to/Thanatos` with your actual config and binary paths.

You can also run with just:

```bash
python -m thanatos_orchestrator
```
if your environment variables or defaults are set correctly.


### Run without installing (quick dev mode)

If you skipped `python -m pip install -e .`, run from the **project root** using `PYTHONPATH`:

```bash
source .venv/bin/activate
PYTHONPATH=src python -m thanatos_orchestrator
```

---

## Commands

Type `help` inside the CLI to see the full command reference.

Main commands:

- `list`  
  Shows tracked servers and verifies whether their ports are actually listening.

- `start-server`  
  Starts a single Thanatos instance using the next available port triple.

- `stop-server <block> <slot>`  
  Stops a specific instance.

- `start-block`  
  Fills the earliest block up to 3 servers (creates a new block only if needed).

- `stop-block <block>`  
  Stops all servers inside a block.

- `start-n-blocks <n>`  
  Starts N blocks (fills partial blocks first).

- `clear`  
  Clears the terminal and redraws the banner.

- `exit`  
  Stops all servers and exits cleanly.

---

## Port Configuration

The orchestrator allocates ports from your base `thanatos.toml`. Ensure you have:

- `thanatos.login_ports` (list of login ports)
- `thanatos.char_ports` (list of char ports)
- `query.ports` (list of query ports)

Each server instance uses one item from each list using the same index (ascending allocation).

---

## Features

- Automatic ascending port allocation and conflict detection
- Live port listening verification (via `ss`)
- Clean process group management (no orphan processes on exit / Ctrl+C)
- Command history navigation (arrow-up like a normal terminal)
- Graceful shutdown

---

## Troubleshooting

### “externally-managed-environment” (Arch / PEP 668)

You are running `pip` outside a venv. Fix:

```bash
cd /path/to/project-root
python -m venv .venv
source .venv/bin/activate
python -m pip install -e .
```

### Permission denied on Thanatos binary

```bash
chmod +x /path/to/Thanatos
```

### Ports already in use

The orchestrator will skip ports already in use. Make sure your `thanatos.toml` has enough ports defined for the number of instances you want to run.

### `ss` not found

Install `iproute2` (Arch usually has it by default):

```bash
sudo pacman -S iproute2
```

---

**Developed by Arkan Software**  
rhayden@arkansoftware.com
