# Space Captain Documentation

This directory contains documentation about the current state of the Space Captain project. Documentation here should be kept up-to-date with code changes.

## Documentation Structure

- **docs/** - Current project documentation that must be updated when code changes
- **docs/research/** - Research papers, speculative designs, and exploratory documentation that don't need updates

## Table of Contents

### Architecture
- [Architecture Diagram](arch.png) - Visual overview of the system architecture
- [Server Loop and Worker Architecture](main-loop.md) - Detailed breakdown of the main network loop and the phased worker tick cycle
- [MPMC Queue Implementation](mpmc-queue.md) - Detailed explanation of the multi-producer multi-consumer queue
- [Worker Thread Pool](worker-thread-pool.md) - In-depth look at the worker thread pool and message processing

### Build System
- [Traditional Build System](build-system.md) - Comprehensive guide to the build system, including tool checking, packaging, and Docker integration
- [Vendored Libraries](vendored-libraries.md) - How mbedTLS is bundled and isolated in packages
- [Versioning Guide](versioning.md) - Semantic versioning and package naming conventions

### Development Guidelines
- [Code Style Guidelines](code-style.md) - Coding standards and naming conventions

### Game Design
- [Coordinate System Precision](coordinate-precision.md) - Analysis of double-precision coordinates for the game world
- [Speed Table](speed-table.md) - Logarithmic speed scale reference and implementation

### Product Requirements
- [Product Requirements Overview](prd/README.md) - Index of all product requirement documents

### Planning
- [Planning Overview](plan/README.md) - Index of all planning documents

### Research Papers
- [Research Overview](research/README.md) - Index of all research documentation
- [Authenticated Messages over UDP](research/authenticated-messages-over-udp.md) - Security considerations for UDP messaging
- [Balanced Continuous Graph Partition Problem](research/balanced-continous-graph-partition-problem.md) - Algorithm research for game world partitioning
- [DTLS Research](research/dtls-research.md) - Datagram TLS protocol analysis
- [mbedTLS Research](research/mbed-tls-research.md) - TLS library evaluation and implementation notes
- [MMO Architecture Research](research/mmo-architecture-research.md) - Scalable multiplayer architecture patterns
- [MMO State Management Protocols](research/mmo-state-management-protocols.md) - State synchronization approaches
- [ncurses Research](research/ncurses-research.md) - Terminal UI library exploration
- [Network Worker Pool Architecture](research/net-worker-pool-architecture.md) - Concurrent network processing design
- [OIDC for CLI Research](research/oidc-for-cli-research.md) - Authentication flow for command-line clients
- [OpenSSL Research](research/openssl-research.md) - TLS library comparison and analysis

## Contributing

When adding new documentation:
1. Place current project documentation in the main `docs/` directory
2. Place research, proposals, and speculative designs in `docs/research/`
3. Use kebab-case for filenames (e.g., `my-new-topic.md`)
4. Update this README when adding current project documentation
5. Documentation in the main directory must be updated when related code changes