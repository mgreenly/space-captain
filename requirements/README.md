# Space Captain Requirements

This directory contains the requirements documents for each version of Space Captain. The project follows a quarterly release cycle with one minor version per quarter.

## Naming Convention

Requirements files should be named using the following pattern:
```
version-X.Y.Z.md
```

Where:
- `X` = Major version (breaking changes)
- `Y` = Minor version (new features, quarterly releases)
- `Z` = Patch version (bug fixes, typically 0 for requirements docs)

## Release Schedule

The project follows a quarterly release cycle:
- **Q1**: January - March
- **Q2**: April - June
- **Q3**: July - September
- **Q4**: October - December

## Version History

### Active Development
- [Version 0.1.0](version-0.1.0.md) - **2025 Q3** (July - September 2025)
  - Core infrastructure and networking
  - Basic 2D space combat mechanics
  - Server-authoritative state management

### Planned Releases
- Version 0.2.0 - **2025 Q4** (October - December 2025)
  - *Requirements to be defined*
- Version 0.3.0 - **2026 Q1** (January - March 2026)
  - *Requirements to be defined*
- Version 0.4.0 - **2026 Q2** (April - June 2026)
  - *Requirements to be defined*

## Version Planning Guidelines

When creating requirements for a new version:

1. **Scope**: Each minor version should contain 3-4 months of development work
2. **Focus**: Maintain a clear theme or focus area for each release
3. **Dependencies**: Build upon previous versions incrementally
4. **Testing**: Include specific success criteria and performance targets
5. **Documentation**: Update this README when adding new version requirements

## Version 1.0 Target

The 1.0 release target has not been set yet. The 1.0 release will represent a feature-complete game with:
- Persistence and account systems
- Multiple ship types
- AI/NPC entities
- Expanded gameplay mechanics

## Contributing

When proposing features for future versions:
1. Consider the quarterly timeline
2. Group related features together
3. Ensure technical dependencies are met in earlier versions
4. Keep performance and scalability goals in mind