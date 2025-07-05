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
*   All ships spawn at a random position within a radius of **1.5 × 10¹¹ meters** from the central star.

### Distributed Game Loop & State Management
*   The "game loop" is a **distributed concept**. Each worker thread runs its own update loop at a fixed **4 Hz (250ms tick rate)**. This loop uses a **batched input processing model** to ensure determinism and fairness.
*   **Phased Tick Cycle:** The worker's main loop is divided into a strict sequence of phases to manage concurrency and enable safe, consistent state snapshots for operations like rebalancing.
    1.  **Synchronization:** Workers wait for a start-of-tick signal from the main server thread. This acts as a barrier, ensuring all workers begin the tick simultaneously.
    2.  **Input Processing:** Each worker drains its dedicated, lock-free MPSC command queue, processing all client messages received since the last tick.
    3.  **Game State Update:** The worker simulates physics, updates entity positions, and resolves combat for all entities under its management.
    4.  **State Broadcast Preparation:** The worker determines the necessary state updates for each client it owns (including data for other entities within their Area of Interest).
    5.  **Message Dispatch:** The prepared update messages are enqueued into a global outbound queue for the main network thread to send to clients.
    6.  **Sleep:** The worker calculates the time spent on the tick and sleeps for the remaining duration to maintain the fixed 4 Hz rate.
*   **Entity Identification:** The server is responsible for generating a unique, persistent `entity_id` (`uint64_t`) for each client upon successful connection. This ID is used in all game logic to reference the entity.
*   **Per-Ship State:** All ship data is managed exclusively by its owning worker thread. The core state for each ship includes:
    *   **Coordinates:** `x`, `y` position (`double`).
    *   **Heading:** The direction the ship is facing (`double`).
    *   **Hull Points:** Current structural integrity (starts at 100 for all ships).
    *   **Power Dials:** Four integer settings (0-100) for `speed`, `shields`, `weapons`, and `cloak`.

### Client State Synchronization (Server-Side)
*   For each client, the server tracks `current state`, `acknowledged state`, and a `pending diff accumulator`.
*   **Hybrid State Broadcast with Area of Interest (AoI):** The server uses a hybrid approach for state synchronization:
    *   **Normal Operation:** Sends only the diff between `current state` and `acknowledged state` each tick.
    *   **Fallback Mode:** Sends full state when the client is more than 10 sequences behind or on initial connection.
    *   **Area of Interest:** Updates include changes to the client's own ship plus any entities within a **`5.0e10` meter radius`.
    *   **v0.1.0 Implementation Note:** The diff calculation will be a dummy implementation that always marks all fields as changed, effectively sending full state every tick. This establishes the protocol structure while deferring optimization to future releases. Since full state is always sent, there is no separate FULL_STATE_SYNC message type in v0.1.0.
*   **State Tracking Per Client:**
    *   `current_state`: The authoritative server state for this client.
    *   `current_sequence`: Incrementing sequence number for ordering.
    *   `acked_state`: Last state acknowledged by the client.
    *   `acked_sequence`: Sequence number of last acknowledged state.
    *   `pending_diff`: Accumulates all changes since last ACK (always indicates "all changed" in v0.1.0).

### Spatial Partitioning & Concurrency Model
*   The game world is discretized into a fixed **128x128 grid**.
*   A **dynamic pool of worker threads (defaulting to 32)** shares the load.
*   **New Connection Handling:** 
    *   New client connections are assigned to worker threads using **round-robin** distribution
    *   Connections are flagged as "pending" when initially assigned
    *   The receiving worker only creates data structures for pending connections (no game logic processing)
    *   After the next rebalance cycle, the spatial partitioning system assigns the client to the appropriate worker based on spawn position
    *   The pending flag is cleared, and normal game processing begins
*   **Hilbert Curve Partitioning:** A background thread uses a Hilbert curve to assign contiguous grid regions to workers.
*   **Lock-Free Rebalancing:** The system uses a **double-buffered partition map** and an **atomic swap** to rebalance the load.
*   **Consistent Snapshot:** To ensure an accurate load assessment, the rebalancing thread will acquire a consistent, point-in-time snapshot of all entity positions. This is orchestrated briefly at the start of a tick to prevent data races with moving entities, after which the main simulation continues while the rebalancing calculation proceeds on the static data.

### Inter-Worker Communication
*   **Asynchronous Message Passing** via **lock-free, multiple-producer/single-consumer (MPSC) queues**.

### Combat & Movement
*   **Movement:** Logarithmic speed scale from 1 to 1000.
*   **Weapons:** All attacks are instant beam weapons (no projectile travel time).
*   **Damage Calculation:**
    *   Base damage = weapon_power_dial / 10 (rounded down)
    *   10 points of weapon power destroys 1 point of hull
    *   Example: 75% weapon power (75 points) = 7 hull damage
*   **Shield Absorption:**
    *   Shields absorb damage before hull takes any damage
    *   2 points of shield power absorb 1 point of incoming damage
    *   Shield absorption = shield_power_dial / 2 (rounded down)
    *   Example: 80% shield power (80 points) can absorb 40 damage
    *   Damage overflow: If damage exceeds shield absorption, remainder hits hull
*   **Destruction:** Ship is destroyed when hull points reach 0.
*   **Respawn:** Death requires a client disconnect and reconnect.

### Cloak Mechanics
*   **Visibility Range:** The distance at which a cloaked ship can be detected decreases logarithmically with cloak power.
*   **Detection Formula:** `detection_range = max_range * (1.0 - (cloak_power/100 * 0.9))`
    *   At 0% cloak power: Ship is visible at full AoI range (5.0e10 meters)
    *   At 100% cloak power: Ship is visible at 10% of max range (5.0e9 meters)
    *   Example: At 50% cloak power, detection range is 55% of max range
*   **Area of Interest Impact:** Cloaked ships are only included in STATE_UPDATE messages if they are within their reduced detection range of the observer.

### Client Interface
*   CLI-only (Linux) with an `ncurses`-based display.

### Client-Side Prediction
*   To compensate for network latency and the low server tick rate (4 Hz), the client will implement simple prediction.
*   **Extrapolation:** When the client receives a state update from the server, it will store the entity's position, velocity, and heading. In between server updates, the client will continue to move the entity forward based on its last known state.
*   **Correction:** Upon receiving a new server update, the client will snap the entity to the new authoritative position. For v0.1.0, no sophisticated smoothing (e.g., interpolation) will be implemented. This simple "snap" correction is sufficient to provide a responsive experience while keeping complexity low.

## 4. Technical Requirements & Architecture

### Server Requirements
*   **Platform:** x86_64 and AArch64 Linux.
*   **Concurrency:** Handle **thousands of concurrent client connections**.
*   **Performance:** All worker threads must maintain a **4 Hz tick rate (250ms)**.
*   **Configuration:** In v0.1.0, all configuration parameters (tick rate, worker count, port, etc.) are defined in a shared `config.h` file that is compiled into both client and server. No external configuration files are used in this version.
*   **Graceful Shutdown:** The server must handle `SIGINT` and `SIGTERM` signals to perform a clean shutdown: stop accepting connections, notify clients, join all threads, and release resources.

### Client Requirements
*   **Platform:** Linux x86_64.
*   **Interface:** CLI/terminal with `ncurses` library.

### Network Protocol & Security: DTLS over UDP

*   **Transport Layer:** The protocol is built on top of **UDP** and secured with **Datagram Transport Layer Security (DTLS) 1.2**.
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
    *   **ACKs:** The client acknowledges receipt by sending back just the sequence number. The server uses this to update the client's acknowledged state and clear the pending diff accumulator.
    *   **Diff-Based Updates:** The server normally sends only state differences (diffs) containing changes since the last acknowledged state. This significantly reduces bandwidth usage. **Note:** In v0.1.0, the diff calculation is a placeholder that marks all fields as changed, effectively sending full state each tick while establishing the protocol framework.
    *   **Full State Fallback:** If a client falls more than 10 sequences behind (due to packet loss or processing delays), the server automatically sends a full state update to resynchronize. In v0.1.0, this is handled by the regular STATE_UPDATE message since full state is always sent.
    *   **Ordering:** The client discards any packet with a sequence number less than or equal to the last one it processed, ensuring it only acts on the newest state.

## 5. Network Protocol Message Types

### Message Header Format
All messages include:
- **Protocol Version** (uint16_t): Protocol version (0x0001 for v0.1.0)
- **Message Type** (uint16_t): Identifies the message type
- **Sequence Number** (uint32_t): For ordering and acknowledgment
- **Timestamp** (uint64_t): Unix timestamp in milliseconds
- **Payload Length** (uint16_t): Size of the message payload

### Client-to-Server Messages

#### DIAL_UPDATE (0x0001)
Updates ship power dial settings.
- **Payload**: speed (uint8_t, 0-100), shields (uint8_t, 0-100), weapons (uint8_t, 0-100), cloak (uint8_t, 0-100)

#### MOVEMENT_INPUT (0x0002)
Changes ship heading/direction.
- **Payload**: heading (double, radians)

#### FIRE_WEAPON (0x0003)
Fire weapons at a target entity.
- **Payload**: target_entity_id (uint64_t)

#### STATE_ACK (0x0004)
Acknowledge receipt of state update.
- **Payload**: acknowledged_sequence (uint32_t)

#### HEARTBEAT (0x0005)
Keep-alive message to maintain connection.
- **Payload**: client_timestamp (uint64_t)

### Server-to-Client Messages

#### STATE_UPDATE (0x1001)
Ship state updates for entities within Area of Interest.
- **Payload**: 
  - entity_count (uint16_t)
  - For each entity:
    - entity_id (uint64_t)
    - x_position (double)
    - y_position (double)
    - velocity_x (double)
    - velocity_y (double)
    - heading (double)
    - hull_points (uint16_t)
    - speed_dial (uint8_t)
    - shield_dial (uint8_t)
    - weapon_dial (uint8_t)
    - cloak_dial (uint8_t)

#### ENTITY_DESTROYED (0x1002)
Notification when a ship is destroyed.
- **Payload**: destroyed_entity_id (uint64_t), destroyer_entity_id (uint64_t)

#### DAMAGE_RECEIVED (0x1003)
Notification of damage taken.
- **Payload**: attacker_entity_id (uint64_t), damage_amount (uint16_t)

#### ERROR_RESPONSE (0x1004)
Generic error response for invalid operations.
- **Payload**: 
  - error_code (uint16_t)
  - request_sequence (uint32_t) - sequence number of the request that caused the error
  - error_message_length (uint16_t)
  - error_message (UTF-8 string)

### Connection Management Messages

#### CONNECTION_ACCEPTED (0x2001)
Server accepts connection and assigns entity ID.
- **Payload**: assigned_entity_id (uint64_t), spawn_x (double), spawn_y (double)

#### CONNECTION_REJECTED (0x2002)
Server rejects connection.
- **Payload**: reason_code (uint16_t), reason_text (variable length string)

#### DISCONNECT_NOTIFY (0x2003)
Clean disconnect notification.
- **Payload**: reason_code (uint16_t)

### Protocol Notes
- All messages are sent over DTLS-secured UDP
- Message types use different ranges: 0x0000-0x0FFF (client), 0x1000-0x1FFF (server), 0x2000-0x2FFF (connection)
- v0.1.0 implements dummy diff calculation (always sends full state)
- Clients discard messages with sequence numbers ≤ last processed
- Server tracks current_state, acked_state, and pending_diff per client
- **Client State Inference**: Clients infer state changes from STATE_UPDATE messages:
  - Entity spawned: New entity_id appears that wasn't in previous update
  - Entity departed: Previously known entity_id is no longer in update
  - Damage dealt: Target's hull_points decreased in next STATE_UPDATE
  - Full state sync: Regular STATE_UPDATE serves this purpose in v0.1.0

## 6. Success Criteria

1.  Server maintains a stable **4 tick/second (250ms)** update rate with a load of **2,000 concurrent clients**, demonstrating the architecture's viability for scaling into the thousands.
2.  The custom reliable-UDP protocol maintains state synchronization with minimal packet loss (<1%) on a LAN.
3.  Client-server latency remains **under 50ms** on a LAN environment.
4.  A 24-hour stress test with simulated clients shows **zero memory leaks**.
5.  All functional tests for combat, movement, and state synchronization pass consistently.

## 7. Risks and Mitigation

1.  **Performance at Scale**: The target of supporting **thousands of clients** on a single server is ambitious.
    *   **Mitigation**: The architecture is designed for this scale. Profiling tools (`perf`) will be used iteratively to eliminate bottlenecks.
2.  **Network Instability / Packet Loss**: UDP does not guarantee delivery.
    *   **Mitigation**: The hybrid diff/full-state approach handles packet loss gracefully. Missing diffs are compensated by the automatic full-state fallback when clients fall behind.
3.  **Concurrency Bugs**: Lock-free programming is complex.
    *   **Mitigation**: Use battle-tested lock-free libraries. Employ rigorous testing, sanitizers (TSan), and formal reasoning about memory barriers.
4.  **Network Protocol Evolution**: The initial protocol may be insufficient.
    *   **Mitigation**: A version field will be included in the protocol header from day one.

