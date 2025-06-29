# Project Requirements

## Overview
Space Captain is a client-server MMO project built in C to explore Linux network programming and systems optimization. Version 0.1.0 focuses exclusively on establishing the core infrastructure: efficient networking protocols, concurrent connection handling, message processing, and state management. This release prioritizes building a solid technical foundation over gameplay mechanics and creates a robust platform for future development and optimization experiments.

## Core Features
- Server-authoritative state management
- Clients only receive information visible to their player (no omniscient data)

## Technical Specs
- Server platform: x86_64 and AArch64 Linux
- Efficiently handle 5,000 concurrent client connections
- Networking approach: epoll-based event loop

## MVP (Minimum Viable Product)
- Each client controls a spacecraft in 2D space
- Coordinate system: 100,000,000 x 100,000,000 meters
- Velocities expressed as fractions of the speed of light
- Basic flight mechanics and movement
- Combat system: shooting at other players
- Ship systems: power management, shields, weapons, and cloaking
- Real-time synchronization of all ship states between clients
- Only Linux clients will be supported in this release

## Non-Goals for Version 0.1.0
- Persistence/save games
- AI/NPCs
- Account system/authentication

## Success Criteria
- Server maintains 60 tick/second update rate with 5,000 clients
- Client-server latency under 50ms on LAN
- Zero memory leaks over 24-hour test

## Known Constraints
- Hard delivery date: End of September
- Must run on a single server (no distributed architecture)

## Enhancment Considerations
- Consider SO_REUSEPORT and multiple network threads
- Consider io_uring
- add functional tests for the client and server
