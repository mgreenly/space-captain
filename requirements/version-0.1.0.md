# Version 0.1.0 Requirements

## Overview
Space Captain is a client-server MMO project built in C to explore Linux network programming and systems optimization. Version 0.1.0 focuses exclusively on establishing the core infrastructure: efficient networking protocols, concurrent connection handling, message processing, and state management. This release prioritizes building a solid technical foundation over gameplay mechanics and creates a robust platform for future development and optimization experiments.

## Core Features
- Server-authoritative state management
- Clients only receive information visible to their player (no omniscient data)

## Technical Specs
- Server platform: x86_64 and AArch64 Linux
- Efficiently handle 5,000 concurrent client connections
- Networking approach: epoll-based event loop

## Game World Specifications

### Coordinate System
- 2D space representing a solar system
- Coordinate type: `double` (IEEE 754 double-precision)
- Units: meters
- Maximum radius: 10¹⁵ meters (1 petameter) from origin
- Total dimensions: 2 × 10¹⁵ × 2 × 10¹⁵ meters
- Origin: Star at center (0,0)
- Precision: 0.125 meters at edge, 0.02 mm at spawn orbit

### Ship Specifications
- Single ship type for all players
- Hull structure: 1,000 units
- Power plant: 100 units total
- Power allocation (1-unit increments):
  - Drives (movement/speed)
  - Shields (defense)
  - Weapons (offense)
  - Cloaking (stealth)

### Movement System
- Speeds measured in meters per second (m/s)
- Speed dial: 1-1000 (logarithmic scale)
  - Speed 1: 500 m/s (minimum)
  - Speed 1000: 1.67 × 10¹² m/s (traverse radius in 10 minutes)
- See docs/speed-table.md for complete speed reference

### Spawning
- New ships spawn at random position on circular orbit
- Spawn orbit radius: 1.5 × 10¹¹ meters (150 million km) from star

## MVP (Minimum Viable Product)
- Each client controls a spacecraft in 2D space
- Basic flight mechanics with logarithmic speed control
- Combat system: shooting at other players
- Dynamic power management between ship systems
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
- Target delivery: 2025 Q3 (July - September 2025)
- Must run on a single server (no distributed architecture)

## Enhancement Considerations
- Consider SO_REUSEPORT and multiple network threads
- Consider io_uring

## Testing
- Functional test suite validates server stability with 50 concurrent clients over 30 seconds
- Tests monitor for error messages and ensure all processes remain running

## Thread Safety Improvements
- Add memory barriers or atomic operations where appropriate in queue.c
- Review and document thread safety guarantees for concurrent data structures
