# Event System

A producer-consumer application that generates structured log events, transmits
them via named pipes (Windows) or FIFOs (Linux), and aggregates them by severity
and component with periodic statistics output.

## Prerequisites

| Requirement | Minimum version |
|---|---|
| CMake | 3.11 |
| C++ compiler | C++17 (GCC 8+, MinGW, MSVC 2017+) |
| Internet (first build) | Catch2 v3.5.0 is downloaded automatically via FetchContent |

## Project Structure

```
event_system/
├── include/            # Public headers
│   ├── event.hpp           # Event struct + Severity enum
│   ├── iipc.hpp            # IIPCWriter / IIPCReader interfaces
│   ├── ievent_serializer.hpp
│   ├── ipc.hpp             # IPCWriter / IPCReader (platform impl)
│   ├── ipc_factory.hpp     # Factory functions
│   ├── event_serializer.hpp
│   ├── producer.hpp
│   ├── consumer.hpp
│   └── signal_helper.hpp   # IStopper + CTRL+C handler
├── src/                # Implementations
│   ├── main.cpp
│   ├── producer.cpp
│   ├── consumer.cpp
│   ├── event.cpp
│   ├── event_serializer.cpp
│   ├── ipc.cpp             # Named Pipe / FIFO implementation
│   └── ipc_factory.cpp
├── test/               # Catch2 unit tests
├── doc/uml/            # PlantUML diagrams
│   ├── class_diagram.puml
│   ├── component_diagram.puml
│   └── sequence_diagram.puml
└── CMakeLists.txt
```

## Architecture

```
┌─────────────────────┐        Named Pipe / FIFO        ┌─────────────────────┐
│      Producer       │ ──────────────────────────────► │      Consumer       │
│  (Pipe Server)      │          events (IPC)           │  (Pipe Client)      │
└─────────────────────┘                                 └─────────────────────┘
  generates events                                        filters by severity
  serializes → JSON                                       deserializes JSON
  sends every 500 ms                                      prints statistics
                                                          every 5 seconds
```

See [`doc/uml/sequence_diagram.puml`](doc/uml/sequence_diagram.puml) for the
full application flow, and [`doc/uml/class_diagram.puml`](doc/uml/class_diagram.puml)
for the class structure.

### Start Order

**Producer must be started first.** It creates the named pipe / FIFO and waits
for the consumer to connect. The consumer retries the connection automatically
(up to 50 × 200 ms = 10 seconds).

```
Terminal 1                             Terminal 2
──────────────────────────────────     ──────────────────────────────────
> EventSystem.exe --producer

[IPC] Pipe created.
[IPC] Waiting for consumer...
                                       > EventSystem.exe --consumer
                                       [IPC] Connected to producer.
[IPC] Consumer connected.
[Producer] Starting...
```

### Disconnect Handling

The pipe is kept alive on the producer side — stopping or restarting the
consumer does **not** kill the producer.

#### Consumer disconnects (e.g. CTRL+C on consumer)

1. The producer's next `write()` returns `ERROR_BROKEN_PIPE` / `EPIPE`.
2. `IPCWriter` calls `reconnect()` internally and waits for the next consumer.
3. A new consumer can connect at any time and events resume immediately.

#### Producer disconnects (e.g. CTRL+C on producer)

1. `IPCReader::read()` detects the broken pipe / EOF.
2. The consumer prints `"Producer has disconnected. Shutting down."` and exits cleanly.

```
Producer          Consumer 1 exits      Consumer 2 starts
   │                     │                     │
   │◄═══ events ════════►│                     │
   │                     │── CTRL+C ──────────►│
   │  BROKEN_PIPE → reconnect()                │
   │  waiting for new consumer...              │
   │◄════════════════════════════════════════ connect()
   │◄═══ events ══════════════════════════════►│
```

## Event Format

Each event has four fields and is transmitted as a single-line JSON string:

| Field | Type | Example |
|---|---|---|
| `timestamp` | `YYYY-MM-DD HH:MM:SS.mmm` | `"2026-05-27 14:03:21.042"` |
| `component` | string | `"Database"`, `"API"`, `"Cache"`, `"Auth"`, `"Logging"` |
| `severity` | enum | `"INFO"`, `"WARNING"`, `"ERROR"` |
| `message` | string | `"Connection timeout"` |

**Wire format example:**
```json
{"timestamp":"2026-05-27 14:03:21.042","component":"Database","severity":"ERROR","message":"Connection timeout"}
```

## Event Severity

| Level | Behaviour |
|---|---|
| `INFO` | Filtered out by consumer — not displayed, not counted |
| `WARNING` | Displayed and counted |
| `ERROR` | Displayed and counted |

## Statistics Output

The consumer prints a summary every 5 seconds:

```
======================================================================
EVENT STATISTICS
======================================================================
Total WARNING events: 12
Total ERROR events:   5

Breakdown by Component:
----------------------------------------------------------------------
  [API]
       ERROR: 2
     WARNING: 4
  [Database]
       ERROR: 3
     WARNING: 8
======================================================================
```

## Build

```bash
cmake -B build
cmake --build build
```

> **Note:** The first build downloads Catch2 automatically — an internet
> connection is required.

## Usage

```bash
# Windows
build\Debug\EventSystem.exe --producer [pipe_name]
build\Debug\EventSystem.exe --consumer [pipe_name]

# Linux
./build/EventSystem --producer [pipe_name]
./build/EventSystem --consumer [pipe_name]
```

`pipe_name` defaults to `event_pipe` if omitted. Both sides must use the same name.

## Tests

```bash
cmake --build build --target EventSystemTests
ctest --test-dir build --output-on-failure
```
