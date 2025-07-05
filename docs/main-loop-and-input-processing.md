# Whitepaper: Main Loop Architecture & Input Processing

**Author**: Gemini
**Date**: 2025-07-05
**Status**: Adopted

## 1. Introduction

A critical design decision in any real-time, authoritative server like Space Captain is how to process incoming client messages. The chosen strategy directly impacts determinism, fairness, performance, and code complexity. This document outlines the common industry approaches and details the architecture chosen for the Space Captain server.

## 2. Core Problem: Handling Asynchronous Input

Client inputs arrive at the server asynchronously, influenced by network latency and client-side processing. The server's core game loop, however, runs at a fixed tick rate (4 Hz for v0.1.0). The fundamental challenge is to reconcile the asynchronous nature of network I/O with the synchronous, discrete steps of the game simulation.

## 3. Design Options

There are two primary models for handling client input in this context.

### Option 1: Continuous (Asynchronous) Processing

In this model, a network thread processes a client message as soon as it arrives. This involves immediately reading the message, acquiring a lock on the relevant game state (e.g., the player's ship), updating that state, and releasing the lock.

*   **Pros**:
    *   Potentially offers the lowest possible latency for a single message, as there is no waiting for the next tick.
*   **Cons**:
    *   **Non-Deterministic**: The order of operations is not guaranteed. Two clients firing at the same time might have their actions processed in a different order depending on network conditions, making bugs difficult to reproduce.
    *   **Unfair**: A client with a faster, more stable connection can have their inputs processed more frequently than a client with a high-latency connection.
    *   **Concurrency Complexity**: This model requires complex, fine-grained locking or advanced lock-free data structures to prevent race conditions. This is a significant source of bugs and performance bottlenecks.

### Option 2: Batched (Synchronous) Processing

This is the industry-standard model for authoritative game servers. Network threads are responsible only for receiving messages and placing them into a thread-safe queue. The main game loop, which runs at a fixed tick rate, processes these messages in discrete, predictable batches.

*   **Pros**:
    *   **Deterministic**: All inputs received within one tick interval are processed together in a defined order. This makes the simulation predictable and reproducible.
    *   **Fairness**: All clients are on a level playing field; their inputs are collected and processed for the same tick.
    *   **Simplified Concurrency**: The core game logic can operate on its state without interruption for the duration of a tick, as all inputs have already been gathered. The primary concurrent data structure is the input queue itself, which is a well-understood problem.
    *   **Performance**: Processing messages in batches can improve cache coherency and reduce the overhead of frequent locking/unlocking.

## 4. The Chosen Architecture: Batched Input Processing

For Space Captain, we have formally adopted the **Batched Input Processing** model. This choice prioritizes determinism, fairness, and simplified concurrency, which are critical for building a robust and scalable server.

Our implementation is built around a phased worker loop that operates at a fixed 4 Hz (250ms) tick rate.

### Phased Worker Tick Cycle

The main loop for each worker thread is divided into a strict sequence of non-overlapping phases:

1.  **Synchronization**: At the beginning of a tick, all worker threads wait at a barrier for a start signal from the main server thread. This ensures all workers start their simulation tick at the same time, providing a consistent state for global operations like load balancing.

2.  **Input Processing**: Each worker drains its dedicated, lock-free, multiple-producer/single-consumer (MPSC) command queue. It processes the entire batch of client messages that have accumulated since the previous tick.

3.  **Game State Update**: With the new client inputs applied, the worker runs the game simulation for all entities it manages. This includes updating positions, resolving combat, and handling other game logic.

4.  **State Broadcast Preparation**: The worker identifies which clients need updates and prepares the outgoing messages. For v0.1.0, this includes the client's own ship state plus the state of all other entities within its Area of Interest (AoI).

5.  **Message Dispatch**: The prepared messages are pushed into a global, thread-safe outbound queue. A dedicated network thread reads from this queue to send the UDP datagrams to the clients.

6.  **Sleep**: The worker calculates the time spent on the tick and sleeps for the remaining duration to maintain the fixed 4 Hz rate.

This phased, batched approach provides a highly stable and predictable foundation for the game simulation, making it easier to debug, test, and scale.
