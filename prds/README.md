# Space Captain Product Requirements Documents

This directory contains the Product Requirements Documents (PRDs) for each version of Space Captain. The project follows a quarterly release cycle with one minor version per quarter.

## Important Notice

**All future release PRDs (0.2.0 and beyond) are pure speculation based on current planning and are likely to change significantly as development progresses.**

## Development Plan

### Active Development
- [Version 0.1.0](version-0.1.0.md) - **2025 Q3** (July - September 2025)
  - Technical Demo
  - Core infrastructure and networking
  - CLI/ncurses client with space combat
  - Captain command REPL interface
  - Weekly development schedule with Sunday milestones

### Planned Releases (Speculative)
- [Version 0.2.0](version-0.2.0.md) - **2025 Q4** (October - December 2025)
  - TLS security implementation
  - OIDC authentication with Google and Apple
  - Cross-platform build system
  - CLI client support for Linux, macOS, and Windows
- [Version 0.3.0](version-0.3.0.md) - **2026 Q1** (January - March 2026)
  - Linux graphical client with Vulkan
  - Feature parity with CLI version
- [Version 0.4.0](version-0.4.0.md) - **2026 Q2** (April - June 2026)
  - MacOS graphical client with Metal
- [Version 0.5.0](version-0.5.0.md) - **2026 Q3** (July - September 2026)
  - Windows graphical client with DirectX
- [Version 0.6.0](version-0.6.0.md) - **2026 Q4** (October - December 2026)
  - Persistent world foundations
  - Crew progression, economy, tech progression
  - Resource gathering and manufacturing

## Version Planning Guidelines

When creating PRDs for a new version:

1. **Scope**: Each minor version should contain 3 months of development work
2. **Focus**: Maintain a clear theme or focus area for each release
3. **Dependencies**: Build upon previous versions incrementally
4. **Testing**: Include specific success criteria and performance targets
5. **Documentation**: Update this README when adding new version PRDs

## Version 1.0 Target

The 1.0 release target has not been set yet. The 1.0 release will represent a feature-complete game with:
- Persistence and account systems
- Multiple ship types
- AI/NPC entities
- Expanded gameplay mechanics

## Naming Convention

PRD files should be named using the following pattern:
```
version-X.Y.Z.md
```

Where:
- `X` = Major version (breaking changes)
- `Y` = Minor version (new features, quarterly releases)
- `Z` = Patch version (bug fixes, typically 0 for PRD docs)

## PRD Document Structure

Each PRD should contain the following sections in order:

1. **Title and Header** - Version number, name, status, target release, and theme
2. **Overview** - High-level description of what the release accomplishes
3. **Goals** - Numbered list of key objectives (typically 3-5 items)
4. **Features** - Detailed breakdown of functionality organized by category
5. **Technical Requirements** - Platform support, dependencies, and performance targets
6. **Testing Requirements** - Functional, performance, and integration testing approach
7. **Migration Path** - Compatibility considerations and documentation needs
8. **Success Criteria** - Measurable goals to determine release success
9. **Risks and Mitigation** - Known challenges and mitigation strategies
10. **Development Schedule** - Weekly milestones for tracking progress (required - Sunday milestones for the target quarter)
11. **Future Considerations** - Non-goals and preparation for subsequent releases

This structure ensures consistency across versions and comprehensive coverage of all aspects needed for successful release planning.