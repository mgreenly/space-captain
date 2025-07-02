# Space Captain FAQ

This directory contains frequently asked questions and detailed explanations about various aspects of the Space Captain project.

## Table of Contents

### Architecture
- [Architecture Diagram](arch.png) - Visual overview of the system architecture
- [Main Server Loop](main-loop.md) - High-level overview of the epoll loop, message queue, and worker threads
- [MPMC Queue Implementation](mpmc-queue.md) - Detailed explanation of the multi-producer multi-consumer queue
- [Worker Thread Pool](worker.md) - In-depth look at the worker thread pool and message processing

### Build System
- [Unified C Project Structure](unified-c-project-structure.md) - Explains the unified build approach used in this project

## Contributing

When adding new FAQ documents:
1. Use kebab-case for filenames (e.g., `my-new-topic.md`)
2. Add a link to the document in this README
3. Organize links by category as the FAQ grows