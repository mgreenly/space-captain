# Space Captain Documentation

This directory contains documentation and detailed explanations about various aspects of the Space Captain project.

## Table of Contents

### Architecture
- [Architecture Diagram](arch.png) - Visual overview of the system architecture
- [Server Loop and Worker Architecture](main-loop.md) - Detailed breakdown of the main network loop and the phased worker tick cycle
- [MPMC Queue Implementation](mpmc-queue.md) - Detailed explanation of the multi-producer multi-consumer queue
- [Worker Thread Pool](worker-thread-pool.md) - In-depth look at the worker thread pool and message processing

### Build System
- [Unified C Project Structure](unified-c-project-structure.md) - Explains the unified build approach used in this project

### Development Guidelines
- [Code Style Guidelines](code-style.md) - Coding standards and naming conventions

### Game Design
- [Coordinate System Precision](coordinate-precision.md) - Analysis of double-precision coordinates for the game world
- [Speed Table](speed-table.md) - Logarithmic speed scale reference and implementation

### Research
- [Net-Worker Pool Architecture](net-worker-pool-architecture.md) - A 3-tier server model for extreme I/O scalability and simplified rebalancing
- [DTLS for Real-Time Game Traffic](dtls-research.md) - Research summary on securing UDP with DTLS and selecting a crypto library
- [MMO Architecture Research](mmo-architecture-research.md) - High-level architecture patterns and design considerations for MMO systems
- [MMO State Management and Protocols](mmo-state-management-protocols.md) - Technical white paper on MMO global state management and client-server protocols
- [Balanced Continuous Graph Partition Problem](balanced-continous-graph-partition-problem.md) - Research on graph partitioning algorithms for distributed game state
- [Authenticated Messages over UDP](authenticated-messages-over-udp.md) - Security considerations for UDP-based game protocols
- [Mbed TLS Integration Research](mbed-tls-research.md) - Analysis of integrating mbed-tls library for TLS encryption with certificate pinning
- [OpenSSL Integration Research](openssl-research.md) - Analysis of integrating OpenSSL for TLS encryption with certificate pinning
- [OIDC Authentication Research](oidc-for-cli-research.md) - OAuth 2.0 + OIDC authentication integration for CLI clients
- [NCurses Research](ncurses-research.md) - Comprehensive guide to NCurses for terminal UI development

## Contributing

When adding new documentation:
1. Use kebab-case for filenames (e.g., `my-new-topic.md`)
2. Add a link to the document in this README
3. Organize links by category as the documentation grows