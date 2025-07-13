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

### Research Papers
See [research/](research/) directory for:
- Architecture research and proposals
- Security and authentication studies
- Network protocol analysis
- UI/UX exploration
- Speculative designs and future features

## Contributing

When adding new documentation:
1. Place current project documentation in the main `docs/` directory
2. Place research, proposals, and speculative designs in `docs/research/`
3. Use kebab-case for filenames (e.g., `my-new-topic.md`)
4. Update this README when adding current project documentation
5. Documentation in the main directory must be updated when related code changes