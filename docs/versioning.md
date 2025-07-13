# Space Captain Versioning Guide

This document describes the versioning scheme used by the Space Captain project and how it translates to package naming conventions.

## Semantic Versioning

Space Captain follows [Semantic Versioning 2.0.0](https://semver.org/) (SemVer) for version management.

### Version Format

Our version format is: `MAJOR.MINOR.PATCH[-preRELEASE][+METADATA]`

Where:
- **MAJOR**: Incremented for incompatible API changes
- **MINOR**: Incremented for backward-compatible functionality additions
- **PATCH**: Incremented for backward-compatible bug fixes
- **preRELEASE**: Optional pre-release identifier (e.g., `-pre1`, `-pre2`)
- **METADATA**: Always includes the git commit SHA (e.g., `+9728a03`)

### Version Components

Version components are stored in separate files:
- `.vMAJOR` - Major version number
- `.vMINOR` - Minor version number
- `.vPATCH` - Patch version number
- `.vPRE` - Pre-release number (empty for stable releases)
- `.vRELEASE` - Package release number

The git commit SHA is automatically appended as build metadata.

### Version Management Commands

```bash
# Increment version components
make major-bump    # X.0.0 - Resets minor, patch, and pre-release
make minor-bump    # 0.X.0 - Resets patch and pre-release
make patch-bump    # 0.0.X - Preserves pre-release
make pre-bump      # Increment pre-release number
make rel-bump      # Increment package release number

# Reset components
make pre-reset     # Clear pre-release version
make rel-reset     # Reset release number to 1

# View current version
make version       # Display full version string
```

### Version Examples

- `0.1.0+9728a03` - Initial development version
- `1.0.0+abc1234` - First stable release
- `1.1.0-pre1+def5678` - Pre-release for new feature
- `1.1.0+ghi9012` - Stable release with new feature
- `2.0.0+jkl3456` - Major version with breaking changes

## Package Naming Conventions

### Debian Packages

Debian packages follow the [Debian Policy Manual](https://www.debian.org/doc/debian-policy/ch-controlfields.html#version) naming convention:

**Format**: `package-name_version-release_architecture.deb`

**Key Rules**:
- Underscores (`_`) separate package name from version and architecture
- Hyphens (`-`) in version strings are replaced with tildes (`~`) in filenames
- Pre-release versions use `~` to ensure proper version ordering

**Examples**:
```
space-captain-server_0.1.0-1_amd64.deb
space-captain-server_0.1.0~pre1-1_amd64.deb
space-captain-server_1.0.0-2_arm64.deb
```

### RPM Packages

RPM packages follow the [Fedora Packaging Guidelines](https://docs.fedoraproject.org/en-US/packaging-guidelines/Versioning/) and [RPM Packaging Guide](https://rpm-packaging-guide.github.io/):

**Format**: `package-name-version-release.architecture.rpm`

**Key Rules**:
- Hyphens (`-`) separate all components
- Pre-release versions use tildes (`~`) for proper ordering
- No distribution tag in our packages (simplified format)

**Examples**:
```
space-captain-server-0.1.0-1.x86_64.rpm
space-captain-server-0.1.0~pre1-1.x86_64.rpm
space-captain-server-1.0.0-2.aarch64.rpm
```

### Package Version Components

- **Version**: Follows SemVer without the git SHA metadata
- **Release**: Package-specific release number (stored in `.vRELEASE`)
  - Incremented when rebuilding the same version
  - Reset to 1 when version changes

## Practical Examples

### Starting a New Feature

```bash
# Current: 1.0.0+abc1234
make minor-bump     # -> 1.1.0+abc1234
make pre-bump       # -> 1.1.0-pre1+abc1234

# After development and testing
make pre-reset      # -> 1.1.0+def5678
```

### Fixing a Bug

```bash
# Current: 1.1.0+def5678
make patch-bump     # -> 1.1.1+ghi9012
```

### Breaking Changes

```bash
# Current: 1.1.1+ghi9012
make major-bump     # -> 2.0.0+jkl3456
```

### Rebuilding a Package

```bash
# Current: 1.0.0+abc1234, Release: 1
make rel-bump       # Release: 2
# Package: space-captain-server_1.0.0-2_amd64.deb
```

## Version Ordering

Package managers correctly order versions following these rules:
1. `1.0.0~pre1` < `1.0.0~pre2` < `1.0.0`
2. `1.0.0` < `1.0.1` < `1.1.0` < `2.0.0`
3. Release numbers only matter for the same version

## References

- [Semantic Versioning Specification](https://semver.org/)
- [Debian Version Format](https://www.debian.org/doc/debian-policy/ch-controlfields.html#version)
- [RPM Versioning Guidelines](https://docs.fedoraproject.org/en-US/packaging-guidelines/Versioning/)
- [Fedora Package Naming Guidelines](https://docs.fedoraproject.org/en-US/packaging-guidelines/Naming/)