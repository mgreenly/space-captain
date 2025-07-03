# Version 0.4.0 PRD - MacOS Graphical Client

**Status**: SPECULATIVE - Subject to change
**Target Release**: 2026 Q2 (April - June 2026)
**Theme**: MacOS Platform Support

## Overview

Version 0.4.0 extends the graphical client to MacOS, providing native support for Apple silicon and Intel-based Macs. This release maintains feature parity with the Linux graphical client from version 0.3.0.

## Goals

1. Native MacOS graphical client
2. Feature parity with Linux graphical client
3. MacOS-specific optimizations and integrations
4. Maintain consistent user experience across platforms

## Features

### MacOS Client
- [ ] MacOS application bundle
- [ ] Metal graphics API integration
- [ ] Native MacOS window management
- [ ] Retina display support
- [ ] Apple Silicon optimization via Metal Performance Shaders

### Platform Integration
- [ ] MacOS menu bar integration
- [ ] Dock support
- [ ] TODO: MacOS-specific features (notifications, etc.)

### Cross-Platform Considerations
- [ ] Shared codebase with Linux client (except rendering)
- [ ] Abstract rendering interface for Vulkan/Metal
- [ ] Platform-specific shader compilation
- [ ] Unified asset pipeline across platforms

### Packaging and Distribution
- [ ] MacOS .app bundle creation
- [ ] Code signing with Apple Developer certificate
- [ ] Notarization for Gatekeeper approval
- [ ] Homebrew cask formula for distribution
- [ ] Automated Homebrew tap maintenance
- [ ] Distribution exclusively through Homebrew

## Technical Requirements

### Platform Support
- MacOS 11.0 (Big Sur) or later
- Intel and Apple Silicon architectures
- TODO: Specific hardware requirements

### Dependencies
- MacOS SDK with Metal framework
- Xcode for Metal shader compilation
- Code signing certificate for distribution
- Homebrew for package distribution

### Performance Targets
- TODO: Performance parity with Linux client
- TODO: Battery life considerations
- TODO: Memory usage on MacOS

## Testing Requirements

### Platform Testing
- [ ] Intel Mac compatibility
- [ ] Apple Silicon compatibility
- [ ] Multiple MacOS versions
- [ ] TODO: Automated testing on MacOS

### Integration Testing
- [ ] Cross-platform server compatibility
- [ ] Feature parity verification
- [ ] TODO: MacOS-specific test cases

## Migration Path

### Compatibility
- Server remains unchanged from 0.3.0
- Network protocol compatibility maintained
- Linux client continues to function

### Documentation
- [ ] MacOS client installation guide
- [ ] Platform-specific troubleshooting
- [ ] TODO: Cross-platform user guide

## Success Criteria

1. MacOS users can play with same features as Linux users
2. Native MacOS look and feel
3. TODO: App Store approval (if applicable)
4. TODO: Performance benchmarks

## Risks and Mitigation

1. **MacOS Development Environment**: TODO
2. **App Store Requirements**: TODO
3. **Cross-Platform Code Maintenance**: TODO

## Development Schedule

### Q2 2026 Weekly Milestones

#### April 2026
- **April 5**: TBD
- **April 12**: TBD
- **April 19**: TBD
- **April 26**: TBD

#### May 2026
- **May 3**: TBD
- **May 10**: TBD
- **May 17**: TBD
- **May 24**: TBD
- **May 31**: TBD

#### June 2026
- **June 7**: TBD
- **June 14**: TBD
- **June 21**: TBD
- **June 28**: TBD

## Future Considerations

This release prepares for Windows support in 0.5.0 by establishing cross-platform architecture.