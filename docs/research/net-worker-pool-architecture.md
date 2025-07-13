# Whitepaper: The Net-Worker Pool Architecture

**Author**: Gemini
**Date**: 2025-07-05
**Status**: Research - Proposed Alternative

## 1. Overview

This document describes a "Net-Worker Pool" architecture, a three-tier server model designed for extreme scalability and simplified logical rebalancing in a distributed environment. It separates the core server functions into three distinct, independently scalable thread pools: an **Acceptor Thread**, a **Net-Worker Pool** for I/O, and a **Game-Worker Pool** for logic.

This model is a powerful alternative to the currently adopted two-tier architecture and is documented here for future consideration.

## 2. Architectural Components

### Tier 1: The Acceptor Thread

*   **Responsibility**: A single thread dedicated exclusively to accepting new TCP/UDP connections.
*   **Operation**: It runs a simple `accept()` loop on the main listening socket.
*   **Connection Handoff**: Upon accepting a new client connection, it immediately hands the socket file descriptor (`fd`) off to a thread in the Net-Worker Pool. The handoff strategy is typically simple round-robin to ensure an even initial distribution of connections.

### Tier 2: The Net-Worker Pool

*   **Responsibility**: This pool of threads is responsible for all direct network I/O. Each Net-Worker is a self-contained I/O engine.
*   **Connection Pinning**: When a Net-Worker receives a socket `fd` from the Acceptor, that connection is **pinned** to the Net-Worker for its entire lifetime. It never moves.
*   **`epoll` Loop**: Each Net-Worker runs its own independent `epoll` loop, managing only the set of clients pinned to it.
*   **Message Handling**:
    *   **Inbound**: Reads data from sockets, deserializes it into a complete game message, and determines which Game-Worker is responsible for the entity associated with the message. It then enqueues the message for the correct Game-Worker.
    *   **Outbound**: Listens for "send" commands from the Game-Worker pool. When it receives a command to send data to a specific client, it serializes the data and writes it to the appropriate socket in its `epoll` set.

### Tier 3: The Game-Worker Pool

*   **Responsibility**: This is the existing pool of worker threads responsible for all core game simulation and logic.
*   **I/O Agnostic**: These threads are completely abstracted away from the network. They never touch a socket, an `epoll` instance, or any networking APIs.
*   **Operation**: They operate on the same phased, fixed-tick-rate loop as in the current architecture:
    1.  Pull a batch of input messages from their command queue.
    2.  Process the messages and update the game state (positions, combat, etc.).
    3.  Prepare state update messages for clients.
    4.  Instead of sending directly, they determine which Net-Worker handles the target client and enqueue a "send" command to that Net-Worker.

## 3. Data Flow and Communication

Communication between the tiers is handled entirely by lock-free queues.

### Inbound Message Flow (Client to Server)

1.  A client's message arrives at its pinned **Net-Worker** (e.g., NW#3).
2.  NW#3's `epoll` loop wakes. It reads the data and forms a message for `entity_id: 123`.
3.  NW#3 consults a global, thread-safe mapping to find that `entity_id: 123` is managed by **Game-Worker** #7 (GW#7).
4.  NW#3 enqueues the message into GW#7's input queue.
5.  GW#7's game loop eventually processes the message.

### Outbound Message Flow (Server to Client)

1.  GW#7 finishes its simulation tick and determines it needs to send a state update to `entity_id: 123`.
2.  GW#7 consults a mapping to find that `entity_id: 123`'s connection is handled by **NW#3**.
3.  GW#7 creates a "send command" containing the data and enqueues it into NW#3's command queue.
4.  NW#3's `epoll` loop processes the command and writes the data to the client's socket.

## 4. The Rebalancing Advantage

The primary advantage of this architecture is the elegant solution to dynamic load rebalancing.

In this model, an entity's "state" can be moved without ever touching its network connection. To move `entity_id: 123` from GW#7 to GW#15:

1.  The system pauses or locks the specific entity's state.
2.  The state data for `entity_id: 123` is transferred from the memory of GW#7 to GW#15.
3.  A single pointer in the global `entity_id -> game_worker_id` map is atomically updated to point to GW#15.

That's it. The difficult problem of handing off a live socket between threads is completely avoided. The pinned Net-Worker #3 seamlessly begins queueing messages for GW#15 instead of GW#7 on the very next message it receives.

## 5. Analysis and Trade-Offs

| Feature | 2-Tier (Current) | 3-Tier (Net-Worker) |
| :--- | :--- | :--- |
| **Scalability** | Good | **Excellent** |
| **Latency** | **Lower** (fewer queue hops) | Higher |
| **Complexity** | Moderate | **High** |
| **Rebalancing** | Moderately Complex | **Simpler Logic** |
| **Separation of Concerns**| Good | **Excellent** |

### Pros
*   **Massive I/O Scalability**: The Net-Worker pool can be scaled independently to handle enormous numbers of connections.
*   **Simplified Rebalancing**: Avoids the primary complexity of rebalancing (socket handoffs).
*   **Architectural Purity**: Provides a very clean and complete separation between I/O and game logic.

### Cons
*   **Increased Latency**: The two required queue hops (Net->Game, Game->Net) for every client interaction add measurable latency compared to a more direct model.
*   **Higher Implementation Complexity**: Requires managing two distinct thread pools, more inter-thread communication queues, and a high-throughput global mapping service.

## 6. Conclusion

The Net-Worker Pool architecture is a powerful and highly scalable pattern suitable for massive online games that need to support tens or hundreds of thousands of concurrent connections while maintaining dynamic load balancing.

However, for the goals of Space Captain v0.1.0 ("thousands" of clients), it represents a premature optimization. The additional complexity and inherent latency are not justified at this stage. The currently adopted two-tier model provides a better balance of performance, simplicity, and scalability for the project's immediate goals. This whitepaper should be preserved for future reference if the project's scale grows to a point where the I/O thread in the two-tier model becomes a proven bottleneck.
