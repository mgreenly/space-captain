# Space Captain: Project Overview

This document provides a high-level summary of the Space Captain project, synthesizing information from the main [README.md](../README.md), the [product requirements documents (PRDs)](../prds/), and the detailed [technical documentation](../docs/).

## 1. High-Level Vision

Space Captain is a client-server MMO project built in C as a technical exploration of Linux network programming, systems optimization, and game design. The project's long-term goal is to create a game that blends elements of Eve Online, Dwarf Fortress, and Factorio.

The development is methodical, prioritizing robust infrastructure and incremental feature development. It will evolve from a foundational technical demo into a persistent world with a rich feature set including crew progression, a player-driven economy, and manufacturing systems.

## 2. Core Architecture & Technology

The project is built on a high-performance, scalable architecture designed to handle thousands of concurrent users. For a complete overview, see the [MMO Architecture Research](docs/mmo-architecture-research.md).

### 2.1. Server Architecture
*   **High-Performance Networking**: The server uses an `epoll`-based event loop in edge-triggered mode, capable of handling over 5000 concurrent connections. More details can be found in the [Main Server Loop](docs/main-loop.md) document.
*   **Multi-Threaded Design**: A [Worker Thread Pool](docs/worker-thread-pool.md) processes game logic concurrently, decoupled from the main I/O thread via a thread-safe [MPMC Queue](docs/mpmc-queue.md).
*   **Unified C Build**: The project uses a ["unity build"](docs/unified-c-project-structure.md) structure where source files are included directly into main compilation units, simplifying the build process and allowing for more aggressive compiler optimizations.
*   **Spatial Partitioning**: The game world is divided into a grid, and a [Hilbert Curve partitioning algorithm](docs/balanced-continous-graph-partition-problem.md) dynamically assigns regions to worker threads to balance the load using lock-free techniques.

### 2.2. Network Protocol
*   **Custom Reliable UDP**: The game will use a custom protocol built on UDP to minimize latency. The design includes a four-way handshake and is detailed in the [Authenticated Messages over UDP](docs/authenticated-messages-over-udp.md) whitepaper.
*   **Security**: Future versions will implement TLS encryption (see [OpenSSL Research](docs/openssl-research.md) and [mbed-tls-research](docs/mbed-tls-research.md)) and [OIDC Authentication](docs/oidc-for-cli-research.md) for secure logins.

### 2.3. Client Architecture
*   **Initial Client (v0.1.0)**: A command-line interface (CLI) built with `ncurses`. See the [NCurses Research](docs/ncurses-research.md) for technical details.
*   **Graphical Clients (v0.3.0+)**: The roadmap includes native graphical clients for Linux (Vulkan), macOS (Metal), and Windows (DirectX).

## 3. Gameplay & World Design

### 3.1. Coordinate System & Scale
*   The game takes place in a 2D solar system with a radius of 1 petameter (10¹⁵ meters). The implications of this scale are analyzed in the [Coordinate System Precision](docs/coordinate-precision.md) document.
*   A logarithmic [Speed Table](docs/speed-table.md) allows for meaningful travel times across these vast distances.

### 3.2. State Management & Synchronization
*   **Server-Authoritative**: The server is the single source of truth for all game state.
*   **Synchronization Techniques**: The architecture relies on standard industry techniques like interest management, client-side prediction, and entity interpolation to provide a smooth and responsive player experience. These concepts are explored in the [MMO State Management Protocols](docs/mmo-state-management-protocols.md) whitepaper.

## 4. Development Roadmap Summary

The project follows a quarterly release cycle, detailed in the [PRD README](prds/README.md). Each version builds upon the last.

*   **[v0.1.0: Core Infrastructure & Combat (Q3 2025)](prds/version-0.1.0.md)**
    *   Focus: Establish the core server architecture, reliable-UDP protocol, and a functional `ncurses`-based CLI client.
    *   Goal: Demonstrate stable 2D space combat with thousands of concurrent connections.

*   **[v0.2.0: Security & Cross-Platform Support (Q4 2025)](prds/version-0.2.0.md)**
    *   Focus: Implement TLS encryption and OIDC authentication.
    *   Goal: Port the CLI client to Linux, macOS, and Windows.

*   **[v0.3.0: Linux Graphical Client (Q1 2026)](prds/version-0.3.0.md)**
    *   Focus: Create a native Linux graphical client using Vulkan.
    *   Goal: Achieve feature parity with the CLI client in a graphical environment.

*   **[v0.4.0: macOS Graphical Client (Q2 2026)](prds/version-0.4.0.md)**
    *   Focus: Port the graphical client to macOS using Metal.
    *   Goal: Provide a native experience on Apple hardware.

*   **[v0.5.0: Windows Graphical Client (Q3 2026)](prds/version-0.5.0.md)**
    *   Focus: Complete the graphical client trilogy with a Windows port using DirectX.
    *   Goal: Achieve full cross-platform graphical client support.

*   **[v0.6.0: Persistent World Foundation (Q4 2026)](prds/version-0.6.0.md)**
    *   Focus: Transition from a session-based game to a persistent MMO.
    *   Goal: Implement foundational systems for player accounts, crew progression, economy, and manufacturing.

This structured approach ensures that a robust and scalable technical foundation is in place before complex gameplay features are added, mitigating technical risk and paving the way for a rich, expansive MMO experience.