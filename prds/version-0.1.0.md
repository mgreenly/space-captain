# Version 0.1.0 PRD - Technical Demo

**Status**: Active Development
**Target Release**: 2025 Q3 (July - September 2025)
**Theme**: Core Infrastructure and Space Combat

## Overview

Space Captain is a client-server MMO project built in C to explore Linux network programming and systems optimization. Version 0.1.0 focuses exclusively on establishing the core infrastructure: efficient networking protocols, concurrent connection handling, message processing, and state management. This release prioritizes building a solid technical foundation over gameplay mechanics and creates a robust platform for future development and optimization experiments.

## Goals

1. Establish core client-server architecture
2. Implement efficient networking with 5,000 concurrent connections
3. Create basic 2D space combat gameplay
4. Build CLI/ncurses interface with captain command REPL
5. Demonstrate server-authoritative state management

## Features

### Core Infrastructure
- Server-authoritative state management
- Clients only receive information visible to their player (no omniscient data)
- Epoll-based event loop for efficient connection handling
- Multi-threaded message processing with worker pool
- Basic logging system for debugging and monitoring
- Structured log levels (ERROR, WARN, INFO, DEBUG)
- Log rotation and file management

### Game World
- 2D space representing a solar system
- Coordinate system using double precision (10¹⁵ meter radius)
- Star at center (0,0)
- Ships spawn at 1.5 × 10¹¹ meters from star

### Ship Systems
- Single ship type for all players
- Hull structure: 1,000 units
- Power plant: 100 units total
- Power allocation (1-unit increments):
  - Drives (movement/speed)
  - Shields (defense)
  - Weapons (offense)
  - Cloaking (stealth)

### Movement Mechanics
- Logarithmic speed scale: 1-1000
- Speed 1: 500 m/s (minimum)
- Speed 1000: 1.67 × 10¹² m/s (traverse radius in 10 minutes)
- See docs/speed-table.md for complete speed reference

### Combat System
- Basic shooting mechanics
- Hull damage and destruction
- Death requires disconnect/reconnect

### Client Interface
- CLI-only (Linux)
- ncurses-based display showing:
  - List of nearby contacts
  - Current target information
  - Power distribution display
  - Command REPL for captain orders
- Client settings persistence:
  - Connection preferences (server address/port)
  - Display preferences
  - Command history
  - Key bindings (if applicable)
  - Settings stored in ~/.spacecaptain/config

## Technical Requirements

### Server Requirements
- Platform: x86_64 and AArch64 Linux
- Handle 5,000 concurrent client connections
- Maintain 60 tick/second update rate
- Single server architecture (no distribution)

### Client Requirements
- Linux x86_64
- CLI/terminal interface
- ncurses library for display

### Network Protocol
- TCP with custom binary protocol
- Fixed 8-byte header (type + length)
- Message types: echo, reverse, time (initial implementation)

## Testing Requirements

### Functional Testing
- Server stability with 50 concurrent clients over 30 seconds
- Message processing correctness
- State synchronization accuracy
- Memory leak detection over 24-hour test

### Performance Testing
- 5,000 concurrent connections
- 60 Hz tick rate maintenance
- Client-server latency under 50ms on LAN

### Thread Safety
- Queue operations verification
- Worker pool concurrency testing
- State management race condition checks

## Migration Path

### Compatibility
- First release - no migration required
- Establishes baseline protocol for future versions
- Foundation for future client implementations

### Documentation
- [ ] Server setup and configuration guide
- [ ] Client installation instructions
- [ ] Basic gameplay tutorial

## Success Criteria

1. Server maintains 60 tick/second with 5,000 clients
2. Client-server latency under 50ms on LAN
3. Zero memory leaks over 24-hour test
4. All functional tests pass consistently
5. Basic space combat gameplay functions correctly

## Risks and Mitigation

1. **Performance at Scale**: May not reach 5,000 clients initially
   - Mitigation: Iterative optimization, profiling tools
2. **Thread Safety Issues**: Concurrent access bugs
   - Mitigation: Thorough testing, memory barriers where needed
3. **Network Protocol Evolution**: Protocol may need changes
   - Mitigation: Version field in header for compatibility

## Development Schedule

### Q3 2025 Weekly Milestones

#### July 2025
- **July 6**: TBD
- **July 13**: TBD
- **July 20**: TBD
- **July 27**: TBD

#### August 2025
- **August 3**: TBD
- **August 10**: TBD
- **August 17**: TBD
- **August 24**: TBD
- **August 31**: TBD

#### September 2025
- **September 7**: TBD
- **September 14**: TBD
- **September 21**: TBD
- **September 28**: TBD

## Future Considerations

### Non-Goals for 0.1.0
- Persistence/save games
- AI/NPCs
- Account system/authentication
- Graphical interface

### Enhancement Opportunities
- Consider SO_REUSEPORT and multiple network threads
- Consider io_uring for improved I/O performance
- Prepare architecture for graphical clients in 0.2.0