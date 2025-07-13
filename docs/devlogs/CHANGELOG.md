# Changelog: Last 21 Days

This document provides a high-level overview of the significant changes to the Space Captain project over the last 21 days. Changes are grouped by functional area rather than chronologically.

## 1. Infrastructure, Packaging, and Deployment

This period saw a major overhaul of the project's infrastructure, focusing on standardization, containerization, and ease of deployment.

-   **Docker-based Packaging:** The build system was enhanced to produce `.deb` (Debian) and `.rpm` (Red Hat) packages using Docker. This ensures a consistent and reproducible build environment for creating distributable packages for major Linux distributions.
-   **Semantic Versioning:** A robust semantic versioning system (`major.minor.patch-prerelease+release`) was implemented via the Makefile, allowing for standardized version bumps and package naming.
-   **Deployment & Configuration:** Infrastructure scripts were reorganized for clarity. The server now intelligently searches for TLS certificates in standard system locations (`/etc/space-captain/`), a local `.secrets/` directory, or via environment variables, simplifying deployment.

## 2. Core Server Architecture & Networking

The server's networking core was significantly enhanced for security, performance, and robustness.

-   **DTLS Integration:** The server now uses DTLS (Datagram Transport Layer Security) for all connections, ensuring that communication between the client and server is secure and encrypted.
-   **Advanced Epoll Handling:** The server's `epoll` event loop was improved to use edge-triggered (`EPOLLET`) notifications and to properly handle peer connection closure with `EPOLLRDHUP`, making the event handling more efficient and robust.
-   **Protocol Implementation:** The core message protocol, including a fixed 8-byte header and initial message types (`MSG_ECHO`, `MSG_REVERSE`, `MSG_TIME`), was implemented. A thread-safe message queue API was also created to manage inter-thread communication.

## 3. Build System & Project Structure

The project's structure and build system were refactored to align with standard C development practices and improve maintainability.

-   **Traditional Compilation Model:** The build system was moved from a monolithic compilation to a traditional model where each `.c` file is compiled into its own object file (`.o`). This improves incremental build times and clarifies dependencies.
-   **Makefile Enhancements:** The `Makefile` was significantly improved with automatic dependency generation (`-MMD -MP`), clear targets for building, running, testing, and formatting, and consistent naming for output binaries (e.g., `sc-server`, `sc-client`).
-   **Directory Reorganization:** Project directories were restructured for clarity. This includes the creation of a `docs/research/` directory to separate speculative research from current project documentation, and the consolidation of sensitive data templates into a `.secrets/` directory.

## 4. Code Quality, Style, and Logging

Efforts were made to standardize code style and improve the developer experience.

-   **Code Formatting:** `clang-format` was integrated into the project (`make fmt`) to enforce a consistent code style across the entire codebase.
-   **Consistent Naming:** A clear function and type naming convention (`sc_<module>_<function>`) was adopted and applied across the project.
-   **Per-File Log Levels:** The logging system was enhanced to allow for log verbosity to be configured on a per-file basis by defining `LOG_LEVEL` before including `log.h`, enabling easier debugging of specific modules.

## 5. Documentation & Game Design

Project documentation was significantly expanded and organized.

-   **Centralized Documentation:** All primary documentation was moved into the `docs/` directory with a clear `README.md` table of contents.
-   **Game Mechanics:** Key game design documents were created, defining the game world's coordinate system, a logarithmic speed scale for ships, and the initial plans for state synchronization.
-   **Agent Guidelines:** A `PROMPT.md` file was created to provide clear instructions and context for AI agents working on the project.
