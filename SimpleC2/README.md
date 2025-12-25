# SimpleC2

A lightweight C2 framework with Python (FastAPI) server and cross-platform agents (C++/Bash).

## Overview

SimpleC2 is a lightweight Command and Control framework written in Python (server) and C++/Bash (agents). It provides a CLI interface to manage listeners, agents, and execute commands on compromised systems.

### Features

- **Multi-listener support**: Run multiple HTTP listeners on different ports
- **Cross-platform agents**: Windows (C++) and Linux (Bash) executors
- **Task queue**: Asynchronous command execution with result collection
- **Payload generation**: Automatic compilation of agents with embedded C2 configuration
- **SQLite persistence**: Track agents, tasks, and results across sessions

## Architecture

```
┌─────────────────┐     HTTP      ┌─────────────────┐
│   C2 Server     │◄────────────► │     Agent       │
│   (Python)      │               │  (C++/Bash)     │
├─────────────────┤               ├─────────────────┤
│  - CLI Menu     │               │  - Beacon       │
│  - Listeners    │               │  - Task Exec    │
│  - Task Queue   │               │  - File Download│
│  - SQLite DB    │               │  - Self-destruct│
└─────────────────┘               └─────────────────┘
```

## Technical Background

### Communication Protocol

The C2 uses a simple HTTP-based protocol with JSON payloads:

1. **Registration** (`POST /reg`): Agent registers with hostname and OS info
2. **Task Polling** (`GET /tasks/{name}`): Agent polls for pending tasks
3. **Result Submission** (`POST /results/{name}`): Agent sends task output (Base64 encoded)
4. **File Download** (`GET /download/{file}`): Agent downloads files from server

### Agent Lifecycle

```
┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐
│ Register │───►│  Poll    │───►│ Execute  │───►│  Report  │
│          │    │  Tasks   │    │  Command │    │  Results │
└──────────┘    └────┬─────┘    └──────────┘    └──────────┘
                     │                                │
                     └────────────────────────────────┘
                              (loop every 3s)
```

## Code Architecture

### Server Components

#### `c2.py`
- Entry point for the C2 server
- Initializes database and starts CLI menu

#### `server/main.py`
- CLI interface using Python's `cmd` module
- Three modes: `C2Menu`, `ListenerMode`, `AgentMode`

#### `server/listeners_manager.py`
- Manages HTTP listeners as separate processes
- Uses `multiprocessing` for concurrent listeners

#### `server/agents_manager.py`
- Agent registration, removal, and task assignment
- Result collection and processing

#### `server/executors_manager.py`
- Payload generation for Windows and Linux
- Compiles C++ agents using Visual Studio toolchain

#### `listener/routes.py`
- FastAPI routes for agent communication
- Handles registration, task distribution, and file serving

#### `db.py`
- SQLite database operations
- Tables: `listeners`, `agents`, `tasks`, `results`

### Agent Components

#### Windows Agent (`executors/win_executor/main.cpp`)
- Native C++ using WinHTTP for communication
- Features:
  - HTTP GET/POST with JSON parsing
  - Command execution via `CreateProcessW`
  - File download and save
  - Base64 encoding for output
  - Console window hiding

#### Linux Agent (`executors/linux_executor.sh`)
- Bash script using `curl` for HTTP requests
- Features:
  - System info collection from `/etc/os-release`
  - Command execution via `eval`
  - Base64 encoding for output
  - File download support

## Database Schema

```sql
-- Listeners table
CREATE TABLE listeners (
    name TEXT PRIMARY KEY,
    port INTEGER,
    created_at TEXT
);

-- Agents table
CREATE TABLE agents (
    name TEXT PRIMARY KEY,
    listener_name TEXT,
    ip TEXT,
    hostname TEXT,
    arrived_at TEXT,
    FOREIGN KEY(listener_name) REFERENCES listeners(name)
);

-- Tasks table
CREATE TABLE tasks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    agent_name TEXT,
    command TEXT,
    issued_at TEXT,
    status TEXT,
    FOREIGN KEY(agent_name) REFERENCES agents(name)
);

-- Results table
CREATE TABLE results (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    task_id INTEGER,
    agent_name TEXT,
    output TEXT,
    received_at TEXT,
    FOREIGN KEY(task_id) REFERENCES tasks(id)
);
```

## Usage

### Requirements

- Python 3.8+
- Visual Studio 2019/2022 (for Windows agent compilation)
- Dependencies: `pip install -r requirements.txt`

### Starting the Server

```bash
python c2.py
```

### CLI Commands

#### Main Menu
| Command | Description |
|---------|-------------|
| `listener` | Enter listener management mode |
| `agent <name>` | Enter agent interaction mode |
| `list agents` | List all registered agents |
| `list listeners` | List all active listeners |
| `payload` | Generate a new agent payload |
| `shutdown` | Kill all listeners and agents |
| `exit` | Exit the C2 |

#### Listener Mode
| Command | Description |
|---------|-------------|
| `start <port>` | Start a new listener on specified port |
| `stop <name>` | Stop a listener by name |
| `exit` | Return to main menu |

#### Agent Mode
| Command | Description |
|---------|-------------|
| `exec <command>` | Execute a command on the agent |
| `download <file>` | Download a file to the agent |
| `clear_tasks` | Clear pending tasks for this agent |
| `quit` | Signal agent to terminate |
| `exit` | Return to main menu |

### Generating Payloads

```
(c2) payload
Listener IP [127.0.0.1]: 192.168.1.100
Listener Port [4444]: 8080
Architecture (x32/x64) [x64]: x64
Output name: myagent
Desired OS (win, linux): win
```

Output is saved to `payload_builds/` directory.

## Example Session

```
C2 CLI. Type ? to list commands.

(c2) listener
Listener mode. Type ? to list commands.

(listener) start 8080
Launched listener_abc123 on port 8080

(listener) exit

(c2) list agents
agent_xyz789 | listener_abc123 | 192.168.1.50 | DESKTOP-PC | 2025-12-25 10:30:00

(c2) agent agent_xyz789
Agent mode. Type ? to list commands.

(agent:agent_xyz789) exec whoami
Exec task sent to 'agent_xyz789': whoami

(agent:agent_xyz789) exec ipconfig /all
Exec task sent to 'agent_xyz789': ipconfig /all

(agent:agent_xyz789) exit
```

## Project Structure

```
SimpleC2/
├── c2.py                    # Entry point
├── db.py                    # Database operations
├── models.py                # Pydantic models
├── requirements.txt         # Python dependencies
├── payload_builds/          # Generated payloads
├── payloads/                # Files for agent download
├── listener/
│   ├── main.py              # FastAPI listener
│   └── routes.py            # HTTP routes
├── server/
│   ├── main.py              # CLI interface
│   ├── agents_manager.py    # Agent management
│   ├── listeners_manager.py # Listener management
│   └── executors_manager.py # Payload generation
└── executors/
    ├── linux_executor.sh    # Linux agent template
    └── win_executor/
        └── main.cpp         # Windows agent source
```

## Limitations

- **No encryption**: All communication is plaintext HTTP
- **No authentication**: Agents connect without verification
- **Basic evasion**: No anti-analysis or obfuscation techniques
- **Single C2 server**: No redundancy or fallback mechanisms

## References

- [Building a Basic C2 from 0xRick](https://0xrick.github.io/misc/c2/)
- [C2 Matrix - Framework Comparison](https://www.thec2matrix.com/)

## Educational Purpose

> ⚠️ **DISCLAIMER**: This code is provided for **educational and research purposes only**. Unauthorized use of this tool against systems you do not own or have explicit permission to test is illegal. The authors are not responsible for any misuse or damage caused by this software.

## Detection & Mitigation

### Indicators of Compromise (IOCs)

1. **Network**:
   - HTTP traffic to non-standard ports with JSON payloads
   - Periodic beacon patterns (default: 3 second intervals)
   - Base64 encoded data in POST requests

2. **Process Behavior**:
   - Hidden console windows (Windows agent)
   - Processes executing commands via `cmd.exe /c`

### Mitigations

1. **Network Monitoring**: Detect periodic HTTP beaconing patterns
2. **EDR**: Monitor for `cmd.exe` spawned by unknown processes
3. **Application Whitelisting**: Block unauthorized executables
4. **Firewall**: Restrict outbound HTTP to known destinations


## License

This project is provided as-is for educational purposes. Use responsibly and ethically.


---

**Last Updated:** December 2025