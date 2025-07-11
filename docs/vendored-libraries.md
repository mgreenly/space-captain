# Vendored Libraries Documentation

This document explains how Space Captain handles vendored libraries, specifically mbedTLS, across different build and packaging scenarios.

## Overview

Space Captain uses mbedTLS for DTLS encryption. Since mbedTLS is not consistently available across all target platforms (notably absent from Amazon Linux 2023 repositories), we vendor and bundle it with our packages.

## Build Process

### Source Management

- mbedTLS source code is cloned to `deps/src/mbedtls/` (git-ignored)
- Version is controlled by `MBEDTLS_VERSION` in the Makefile (currently v2.28.3)
- The `clone-mbedtls` make target ensures source availability on the host system

### Platform-Specific Builds

mbedTLS is built separately for each platform to avoid cross-contamination:

- **Debian**: Built to `deps/build/debian/`
- **Amazon Linux**: Built to `deps/build/amazon/`

The build process:
1. Host system clones mbedTLS source via `make clone-mbedtls`
2. Docker containers or local builds compile mbedTLS to platform-specific directories
3. Binaries are linked against the platform-specific libraries

## Packaging Strategy

### Installation Layout

Both Debian (.deb) and RPM packages use the same installation structure:

```
/usr/bin/space-captain-server          # Wrapper script
/usr/lib/space-captain/
    ├── sc-server-<version>            # Actual binary
    ├── libmbedtls.so.14               # mbedTLS library
    ├── libmbedcrypto.so.7             # mbedTLS crypto library
    └── libmbedx509.so.3               # mbedTLS X.509 library
```

### Wrapper Script

The `/usr/bin/space-captain-server` wrapper script ensures proper library loading:

```bash
#!/bin/bash
export LD_LIBRARY_PATH=/usr/lib/space-captain:$LD_LIBRARY_PATH
exec /usr/lib/space-captain/sc-server-* "$@"
```

This approach:
- Sets `LD_LIBRARY_PATH` to include our bundled libraries
- Executes the actual versioned binary
- Passes through all command-line arguments

### Package Dependencies

- **Debian**: Only depends on `libc6` (removed `libmbedtls14` dependency)
- **RPM**: No mbedTLS dependency declared

## Docker Build Environment

### Builder Images

Two Docker builder images handle compilation:
- `space-captain-debian-builder`: Based on `debian:stable-slim`
- `space-captain-amazon-builder`: Based on `amazonlinux:2023`

### Build Flow

1. Host system ensures mbedTLS source exists: `make clone-mbedtls`
2. Docker container is launched with project mounted at `/workspace`
3. Container runs with host user's UID/GID for proper file ownership
4. mbedTLS is built inside container to `deps/build/<platform>/`
5. Package is created with bundled libraries

### Library Path Resolution

During Docker builds:
- Compile time: `-Ideps/build/<platform>/include` (CFLAGS)
- Link time: `-Ldeps/build/<platform>/lib` (LDFLAGS)
- Runtime: RPATH set to `deps/build/<platform>/lib` via `-Wl,-rpath`

## Library Isolation and Collision Prevention

### How We Avoid System Library Conflicts

The bundled libraries are isolated from system libraries through several mechanisms:

1. **Private Directory**: Libraries are installed in `/usr/lib/space-captain/`, not in system library paths (`/usr/lib`, `/lib`)

2. **Controlled Library Path**: The wrapper script prepends our directory to `LD_LIBRARY_PATH`:
   ```bash
   export LD_LIBRARY_PATH=/usr/lib/space-captain:$LD_LIBRARY_PATH
   ```
   This ensures our libraries are found first, but doesn't affect other programs

3. **Process Isolation**: The `LD_LIBRARY_PATH` modification only affects the Space Captain server process and its children, not the system globally

4. **No ldconfig Registration**: We don't run `ldconfig` or add our directory to `/etc/ld.so.conf.d/`, so system-wide library resolution is unaffected

### Potential Conflicts and Solutions

**Scenario**: System has mbedTLS installed (e.g., libmbedtls.so.14)
- Our binary uses `/usr/lib/space-captain/libmbedtls.so.14` 
- Other programs continue using `/usr/lib/x86_64-linux-gnu/libmbedtls.so.14`
- No conflict occurs due to path isolation

**Scenario**: User sets `LD_LIBRARY_PATH` globally
- Our wrapper script prepends our path, ensuring our libraries are found first
- After our process exits, the user's environment is unchanged

**Scenario**: Another package needs a different mbedTLS version
- Each application uses its own version without interference
- Dynamic linker respects each process's library path configuration

## Directory Structure

The project uses a unified dependency structure:

```
deps/
├── src/              # Third-party source code
│   └── mbedtls/     # mbedTLS source from git
└── build/           # Built libraries
    ├── debian/      # Debian-specific builds
    │   ├── include/ # Header files
    │   └── lib/     # Shared libraries
    └── amazon/      # Amazon Linux builds
        ├── include/ # Header files
        └── lib/     # Shared libraries
```

## Benefits

1. **Self-contained packages**: No external mbedTLS dependency required
2. **Version consistency**: Same mbedTLS version across all platforms
3. **Platform isolation**: No cross-contamination between builds
4. **Simple deployment**: Single package installation with no dependency hunting
5. **No system pollution**: System library paths remain clean
6. **Version independence**: Can use different mbedTLS version than system
7. **Clear organization**: All dependencies in one `deps/` directory

## Maintenance Notes

### Updating mbedTLS Version

1. Update `MBEDTLS_VERSION` in Makefile
2. Run `make clean-all` to remove old builds
3. Rebuild packages for all platforms

### Adding New Platforms

1. Create new Docker builder image (e.g., `Dockerfile.fedora`)
2. Add platform detection in OS-specific settings in Makefile
3. Add architecture mappings if needed
4. Ensure wrapper script pattern is followed

### Troubleshooting

- **Library not found**: Check wrapper script and `LD_LIBRARY_PATH`
- **Version mismatch**: Ensure clean builds with `make clean-all`
- **Permission issues**: Verify Docker UID/GID mapping
- **Build failures**: Check that `deps/src/mbedtls` exists via `make clone-mbedtls`

### Make Targets

- `make clone-mbedtls` - Clone mbedTLS source to `deps/src/mbedtls`
- `make mbedtls` - Build mbedTLS for current platform
- `make clean-all` - Remove all artifacts including `deps/`
- `make package-<os>` - Build package using Docker (debian/amazon)