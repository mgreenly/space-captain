# Server Loop and Worker Architecture

**Status**: Adopted

## 1. High-Level Architecture

The Space Captain server uses a high-performance architecture combining an `epoll`-based event loop on the main thread with a pool of worker threads for game logic processing. This design allows the server to handle thousands of concurrent connections efficiently by separating network I/O from CPU-intensive simulation tasks.

A key element of this architecture is the use of thread-safe, lock-free queues to pass messages from the network thread to the appropriate worker threads.

## 2. The Main Network Thread

The main thread is responsible for all direct network I/O. It runs an edge-triggered `epoll` loop that handles the following:

*   Accepting new client connections.
*   Reading incoming data from all client sockets.
*   Performing minimal parsing to identify message boundaries.
*   Dispatching complete messages to the appropriate worker's input queue.
*   Sending outgoing state updates prepared by the worker threads.

### Key Characteristics:
- **Edge-triggered mode (`EPOLLET`)**: Reduces the number of system calls, but requires the application to drain all available data from the socket.
- **Non-blocking I/O**: The main thread never blocks on network operations, ensuring it can always handle new events.
- **Connection Pooling**: The server pre-allocates thousands of client buffers at startup to avoid `malloc` calls during runtime.

## 3. Worker Thread Architecture

### The Problem: Handling Asynchronous Input

Client inputs arrive at the server asynchronously, influenced by network latency. The server's game loop, however, runs at a fixed tick rate (4 Hz for v0.1.0). The fundamental challenge is to reconcile the asynchronous nature of network I/O with the synchronous, discrete steps of the game simulation. While processing inputs immediately might seem to offer lower latency, it leads to non-deterministic behavior, fairness issues, and extreme concurrency complexity.

### The Solution: Batched Input Processing

To solve this, Space Captain has adopted the industry-standard **Batched Input Processing** model. This model prioritizes determinism, fairness, and simplified concurrency.

The implementation is built around a **phased worker loop** that operates at a fixed 4 Hz (250ms) tick rate.

### Phased Worker Tick Cycle

The main loop for each worker thread is divided into a strict sequence of non-overlapping phases:

1.  **Synchronization**: At the beginning of a tick, all worker threads wait at a barrier for a start signal from the main server thread. This ensures all workers start their simulation tick at the same time, providing a consistent state for global operations like load balancing.

2.  **Input Processing**: Each worker drains its dedicated, lock-free, multiple-producer/single-consumer (MPSC) command queue. It processes the entire batch of client messages that have accumulated since the previous tick.

3.  **Game State Update**: With the new client inputs applied, the worker runs the game simulation for all entities it manages. This includes updating positions, resolving combat, and handling other game logic.

4.  **State Broadcast Preparation**: The worker identifies which clients need updates and prepares the outgoing messages. For v0.1.0, this includes the client's own ship state plus the state of all other entities within its Area of Interest (AoI).

5.  **Message Dispatch**: The prepared messages are pushed into a global, thread-safe outbound queue. The main network thread reads from this queue to send the UDP datagrams to the clients.

6.  **Sleep**: The worker calculates the time spent on the tick and sleeps for the remaining duration to maintain the fixed 4 Hz rate.

### Concrete Scenario

To illustrate the flow, consider a scenario with two players, **Player A** and **Player B**, both managed by the same worker thread. The server is running at 4 ticks per second (250ms per tick).

**Timeline:**
*   `T=0ms`: Tick #100 begins.
*   `T=50ms`: Player A's client sends an input message: `SET_WEAPONS_DIAL(100)`.
*   `T=120ms`: Player B's client sends an input message: `SET_SPEED_DIAL(80)`.
*   `T=200ms`: Player A's client sends another input: `FIRE_WEAPONS`.
*   `T=250ms`: Tick #100 ends. Tick #101 begins.

**Flow through the Phases for Tick #101:**

1.  **Input Queue (Before Tick #101):**
    The server's main network thread receives these three messages asynchronously. It immediately places them into the responsible worker's MPSC queue.

2.  **Phase 1: Synchronization (T=250ms):**
    The worker thread, along with all other workers, waits for and receives the global start signal for Tick #101.

3.  **Phase 2: Input Processing (T=251ms - T=260ms):**
    The worker thread drains its input queue, processing the three messages in order. Player A's weapon dial is set, Player B's speed dial is set, and Player A's fire command is registered.

4.  **Phase 3: Game State Update (T=261ms - T=300ms):**
    The worker simulates the tick: it calculates Player B's new velocity, processes Player A's fire command by creating a projectile, and updates all other entity positions.

5.  **Phase 4 & 5: State Dispatch (T=301ms - T=320ms):**
    The worker prepares and enqueues the state update packets for the main network thread to send out.

The worker then calculates it spent `70ms` on the tick and sleeps for the remaining `180ms` until Tick #102 begins.

## 4. Graceful Shutdown

The server handles `SIGINT` and `SIGTERM` for a graceful shutdown:

1.  A global `shutdown_flag` is set by the signal handler.
2.  The main loop exits on its next iteration.
3.  The server stops accepting new connections.
4.  The worker pool is signaled to stop. Workers finish their current tick, drain their input queues one last time, and then exit.
5.  All remaining resources are freed.
