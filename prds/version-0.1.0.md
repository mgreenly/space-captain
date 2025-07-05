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
*   **Logging**: The server will log to standard streams following Unix conventions. Log messages must follow the format: `YYYY-MM-DD HH:MM:SS <LEVEL>: <message>`.
    *   `DEBUG` and `INFO` levels will be written to **`stdout`**.
    *   `WARN`, `ERROR`, and `FATAL` levels will be written to **`stderr`**.
    *   The application will not handle log files or rotation; this is the responsibility of the service manager (e.g., `systemd`).

### Game World & Coordinate System
*   A 2D space representing a solar system with a **center-origin** at `(0.0, 0.0)`.
*   Coordinate system uses **`double`-precision floating-point numbers**, with a world radius of **1.0e14 meters**.
*   All ships spawn at a fixed distance of **1.5 × 10¹¹ meters** from the central star.

### Distributed Game Loop & State Management
*   The "game loop" is a **distributed concept**. Each worker thread runs its own update loop at a fixed **4 Hz (250ms tick rate)**. This loop uses a **batched input processing model** to ensure determinism and fairness.
*   **Phased Tick Cycle:** The worker's main loop is divided into a strict sequence of phases to manage concurrency and enable safe, consistent state snapshots for operations like rebalancing.
    1.  **Synchronization:** Workers wait for a start-of-tick signal from the main server thread. This acts as a barrier, ensuring all workers begin the tick simultaneously.
    2.  **Input Processing:** Each worker drains its dedicated, lock-free MPSC command queue, processing all client messages received since the last tick.
    3.  **Game State Update:** The worker simulates physics, updates entity positions, and resolves combat for all entities under its management.
    4.  **State Broadcast Preparation:** The worker determines the necessary state updates for each client it owns (including data for other entities within their Area of Interest).
    5.  **Message Dispatch:** The prepared update messages are enqueued into a global outbound queue for the main network thread to send to clients.
*   **Entity Identification:** The server is responsible for generating a unique, persistent `entity_id` (`uint64_t`) for each client upon successful connection. This ID is used in all game logic to reference the entity.
*   **Per-Ship State:** All ship data is managed exclusively by its owning worker thread. The core state for each ship includes:
    *   **Coordinates:** `x`, `y` position (`double`).
    *   **Heading:** The direction the ship is facing (`double`).
    *   **Hull Points:** Current structural integrity.
    *   **Power Dials:** Four integer settings (0-100) for `speed`, `shields`, `weapons`, and `cloak`.

### Client State Synchronization (Server-Side)
*   For each client, the server tracks `server state` and `acknowledged state`.
*   **State Broadcast with Area of Interest (AoI):** For this version, the server will send a state update to each client every tick. This update includes the full state of the client's *own ship*, plus the `ID, x, y, heading` of all other entities within a **`5.0e10` meter radius**. While future versions will send only the diff between the `server state` and `acknowledged state`, the v0.1.0 release will send the complete state in each update.

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

### Network Protocol & Security: DTLS over UDP

*   **Transport Layer:** The protocol is built on top of **UDP** and secured with **Datagram Transport Layer Security (DTLS) 1.3**.
*   **Security Library Abstraction:** To maintain flexibility, the server will use a custom wrapper interface that abstracts the underlying security library.
    *   The initial implementation will use **Mbed TLS**.
    *   The wrapper will be designed to allow for future integration of other libraries like wolfSSL with a compile-time flag.
*   **Cipher Suite Selection:** The DTLS connection will be configured to prioritize performance by selecting cipher suites with the lowest computational cost (e.g., using ChaCha20-Poly1305 or AES-GCM). Security only needs to be guaranteed for the duration of a single game session.
*   **Authentication with Certificate Pinning:**
    *   The server will use a self-signed certificate.
    *   The client will not use a traditional public CA infrastructure. Instead, it will be pre-configured with a hash of the server's public key or certificate.
    *   During the DTLS handshake, the client will verify that the server's certificate matches the pinned hash, preventing man-in-the-middle (MITM) attacks.
*   **Connection Handshake:** The DTLS handshake itself will serve as the connection establishment mechanism, replacing the previous manual four-way handshake. A successful handshake indicates a secure and established session.
*   **Session Management:**
    *   **Keep-Alive:** DTLS has built-in replay protection and session management. The application will send periodic heartbeat messages over the secure channel to ensure the connection remains active.
    *   **Timeout:** The server will disconnect a client if no valid DTLS records are received within a 30-second window.
*   **State Synchronization & Reliability:**
    *   **Sequencing:** The server includes an incrementing sequence number in every state update packet sent to a client over the secure channel.
    *   **ACKs:** The client acknowledges receipt by sending back the full state update it received. This allows the server to set the new acknowledged state for the client without needing to store a history of sent states.
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

