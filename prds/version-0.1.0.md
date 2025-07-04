Of course. Here is the complete, final version of the document, incorporating all the specified details and the latest change to the client count.

---

### **Product Requirements & System Design: Space Captain - Version 0.1.0**

**Status**: Active Development
**Target Release**: Q3 2025 (July - September 2025)
**Theme**: Core Infrastructure and Space Combat

## 1. Overview

Space Captain is a client-server MMO project built in C to explore Linux network programming and high-performance systems design. Version 0.1.0 focuses exclusively on establishing a state-of-the-art core infrastructure capable of handling **thousands of concurrent connections** with high efficiency.

This release will implement a **server-authoritative, distributed, and lock-free architecture** over a **custom reliable-UDP protocol**. The game world will be spatially partitioned and assigned to a dynamic pool of worker threads, enabling massive parallelism. This approach prioritizes building a technically superior foundation for future development, ensuring the system can scale to meet ambitious gameplay goals.

## 2. Goals

1.  Establish a scalable, multi-threaded client-server architecture.
2.  Implement efficient networking to support **thousands of concurrent connections**.
3.  Create basic 2D space combat gameplay to validate the architecture.
4.  Build a functional CLI/ncurses interface with a command REPL.
5.  Demonstrate a robust, server-authoritative, and highly parallel state management system.

## 3. Features

### Core Infrastructure
*   Server-authoritative state management.
*   Clients only receive targeted state updates for objects within their area of interest.
*   `epoll`-based event loop for efficient network I/O.
*   Dynamic worker pool for parallel message processing and state updates.
*   Basic logging system with structured levels and log rotation.

### Game World & Coordinate System
*   A 2D space representing a solar system with a **center-origin** at `(0.0, 0.0)`.
*   Coordinate system uses **`double`-precision floating-point numbers**, with a world radius of **1.0e14 meters**.
*   All ships spawn at a fixed distance of **1.5 × 10¹¹ meters** from the central star.

### Distributed Game Loop & State Management
*   The "game loop" is a **distributed concept**. Each worker thread runs its own update loop at a fixed **4 Hz (250ms tick rate)**.
*   **Entity Identification:** The server is responsible for generating a unique, persistent `entity_id` (`uint64_t`) for each client upon successful connection. This ID is used in all game logic to reference the entity.
*   **Per-Ship State:** All ship data is managed exclusively by its owning worker thread. The core state for each ship includes:
    *   **Coordinates:** `x`, `y` position (`double`).
    *   **Heading:** The direction the ship is facing (`double`).
    *   **Hull Points:** Current structural integrity.
    *   **Power Dials:** Four integer settings (0-100) for `speed`, `shields`, `weapons`, and `cloak`.

### Client State Synchronization (Server-Side)
*   For each client, the server tracks `Current Visible State` and `Last Acknowledged State`.
*   **State Broadcast with Area of Interest (AoI):** For this version, the server will send a state update to each client every tick. This update includes the full state of the client's *own ship*, plus the `ID, x, y, heading` of all other entities within a **`5.0e10` meter radius**.

### Spatial Partitioning & Concurrency Model
*   The game world is discretized into a fixed **128x128 grid**.
*   A **dynamic pool of worker threads (defaulting to 32)** shares the load.
*   **Hilbert Curve Partitioning:** A background thread uses a Hilbert curve to assign contiguous grid regions to workers.
*   **Lock-Free Rebalancing:** The system uses a **double-buffered partition map** and an **atomic swap** to rebalance the load.
*   **Consistent Snapshot:** To ensure an accurate load assessment, the rebalancing thread will acquire a consistent, point-in-time snapshot of all entity positions. This is orchestrated briefly at the start of a tick to prevent data races with moving entities, after which the main simulation continues while the rebalancing calculation proceeds on the static data.

### Inter-Worker Communication
*   **Asynchronous Message Passing** via **lock-free, multiple-producer/single-consumer (MPSC) queues**.

### Combat & Movement
*   **Movement:** Logarithmic speed scale from 1 to 1000.
*   **Combat:** Basic weapon firing, hull damage, and destruction.
*   **Respawn:** Death requires a client disconnect and reconnect.

### Client Interface
*   CLI-only (Linux) with an `ncurses`-based display.

## 4. Technical Requirements & Architecture

### Server Requirements
*   **Platform:** x86_64 and AArch64 Linux.
*   **Concurrency:** Handle **thousands of concurrent client connections**.
*   **Performance:** All worker threads must maintain a **4 Hz tick rate (250ms)**.
*   **Configuration:** The server must load key operational parameters (tick rate, worker count, port, etc.) from an external configuration file (e.g., `server.conf`) at startup.
*   **Graceful Shutdown:** The server must handle `SIGINT` and `SIGTERM` signals to perform a clean shutdown: stop accepting connections, notify clients, join all threads, and release resources.

### Client Requirements
*   **Platform:** Linux x86_64.
*   **Interface:** CLI/terminal with `ncurses` library.

### Network Protocol: Custom Reliable UDP
*   **Transport Layer:** The protocol is built on top of **UDP**.
*   **Connection Handshake:** A session is established via a four-way handshake:
    1.  **Client -> Server:** `MSG_HELLO` (Client requests connection)
    2.  **Server -> Client:** `MSG_CHALLENGE` (Server replies with a unique session token)
    3.  **Client -> Server:** `MSG_RESPONSE` (Client sends the token back)
    4.  **Server -> Client:** `MSG_WELCOME` (Server confirms, assigns an `entity_id`, and adds the client to its active list)
*   **Session Management & Security:**
    *   **Keep-Alive:** Clients must send a periodic `MSG_HEARTBEAT` packet if idle.
    *   **Timeout:** The server will disconnect a client if no valid packets are received within a 30-second window.
    *   **Spoofing Mitigation:** All packets from a connected client must contain the session token assigned during the handshake for validation by the server.
*   **State Synchronization & Reliability:**
    *   **Sequencing:** The server includes an incrementing sequence number in every state update packet sent to a client.
    *   **ACKs:** The client acknowledges receipt by sending an ACK packet containing the sequence number of the message it received.
    *   **Ordering:** The client discards any packet with a sequence number less than or equal to the last one it processed, ensuring it only acts on the newest state.

## 5. Success Criteria

1.  Server maintains a stable **4 tick/second (250ms)** update rate with a load of **at least 1,000 concurrent clients**, demonstrating the architecture's viability for scaling into the thousands.
2.  The custom reliable-UDP protocol maintains state synchronization with minimal packet loss (<1%) on a LAN.
3.  Client-server latency remains **under 50ms** on a LAN environment.
4.  A 24-hour stress test with simulated clients shows **zero memory leaks**.
5.  All functional tests for combat, movement, and state synchronization pass consistently.

## 6. Risks and Mitigation

1.  **Performance at Scale**: The target of supporting **thousands of clients** on a single server is ambitious.
    *   **Mitigation**: The architecture is designed for this scale. Profiling tools (`perf`) will be used iteratively to eliminate bottlenecks.
2.  **Network Instability / Packet Loss**: UDP does not guarantee delivery.
    *   **Mitigation**: For v0.1.0, because the full state is sent every 250ms, a single dropped packet will be corrected by the next successful one.
3.  **Concurrency Bugs**: Lock-free programming is complex.
    *   **Mitigation**: Use battle-tested lock-free libraries. Employ rigorous testing, sanitizers (TSan), and formal reasoning about memory barriers.
4.  **Network Protocol Evolution**: The initial protocol may be insufficient.
    *   **Mitigation**: A version field will be included in the protocol header from day one.

