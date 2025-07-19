# Packaging Guide

This document describes how packaging works in the Space Captain project, including OS-specific considerations and instructions for adding support for new operating systems.

## Overview

Space Captain uses a sophisticated multi-OS packaging system that:
- Supports both DEB (Debian/Ubuntu) and RPM (Amazon Linux/Fedora) package formats
- Uses Docker containers for consistent, reproducible builds across different host systems
- Implements a two-stage RPATH approach for library management
- Bundles mbedTLS libraries with proper dependency isolation

### Key Concepts

1. **OS-Specific Build Directories**: Each OS has its own build directory (`obj/<os>/` and `deps/build/<os>/`) to prevent mixing artifacts between different OS builds.

2. **Docker-Based Building**: Each supported OS has a corresponding Docker container that provides the exact build environment needed for that distribution.

3. **RPATH Management**: During development, binaries use RPATH to find libraries in the build directory. During packaging, RPATH is stripped for security and portability.

4. **Bundled Libraries**: The mbedTLS libraries are bundled in `/usr/lib/space-captain/` with a wrapper script that sets `LD_LIBRARY_PATH`.

## Quick Start

```bash
# Build package for current OS
make package

# Build packages for all supported OSes (host only)
make packages

# Build package for specific OS
make package-debian
make package-ubuntu
make package-amazon
make package-fedora

# Enter OS-specific build environment
make shell-debian
make shell-ubuntu
make shell-amazon
make shell-fedora
```

## Package Structure

### DEB Packages (Debian/Ubuntu)
```
/usr/bin/space-captain-server         # Wrapper script
/usr/lib/space-captain/               # Application binaries and libraries
    sc-server-<version>               # Actual server binary
    libmbedtls.so.*                   # Bundled mbedTLS libraries
    libmbedcrypto.so.*
    libmbedx509.so.*
/usr/lib/systemd/system/              # Systemd service file
    space-captain-server.service
/etc/space-captain/                   # Configuration files
    server.crt
    server.key
```

### RPM Packages (Amazon Linux/Fedora)
Same structure as DEB packages, with RPM-specific metadata and scripts.

## OS-Specific Considerations

### Debian
- **Base Image**: `debian:stable-slim`
- **Package Tool**: `dpkg-deb`
- **Special Considerations**:
  - Uses `postinst` and `prerm` scripts for service management
  - Configuration files marked in `conffiles`
  - No strict RPATH checking by default

### Ubuntu
- **Base Image**: `ubuntu:24.04`
- **Package Tool**: `dpkg-deb` (same as Debian)
- **Special Considerations**:
  - Shares packaging scripts with Debian
  - May have different library versions than Debian
  - Uses same DEB packaging format

### Amazon Linux
- **Base Image**: `amazonlinux:2023`
- **Package Tool**: `rpmbuild`
- **Special Considerations**:
  - RPM-based with less strict RPATH checking
  - Uses `yum` for package management
  - Different system paths than Fedora

### Fedora
- **Base Image**: `fedora:40`
- **Package Tool**: `rpmbuild`
- **Special Considerations**:
  - **Strict RPATH checking** - build will fail if RPATH points to build directory
  - Requires `chrpath` to strip RPATH from binaries
  - Uses `dnf` for package management
  - Most stringent packaging requirements

## Adding Support for a New OS

To add support for a new operating system, you need to modify several components of the build system:

### 1. Create Docker Builder Image

Create `Dockerfile.<os>`:
```dockerfile
FROM <base-image>

# Install build dependencies
RUN <package-manager> install -y \
    gcc \
    make \
    cmake \
    python3 \
    <package-builder> \    # dpkg-dev or rpm-build
    chrpath \               # For RPATH stripping
    gosu \                  # For user switching
    # ... other dependencies

# Copy entrypoint script
COPY scripts/docker-entrypoint-<os>.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
CMD ["/bin/bash"]
```

### 2. Create Docker Entrypoint Script

Create `scripts/docker-entrypoint-<os>.sh`:
```bash
#!/bin/bash
# Docker entrypoint script for <os> builder container

USER_ID=${USER_ID:-1000}
GROUP_ID=${GROUP_ID:-1000}

# Create group and user with matching IDs
groupadd -g $GROUP_ID -o builduser 2>/dev/null || true
useradd -u $USER_ID -g $GROUP_ID -o -m -s /bin/bash builduser 2>/dev/null || true

# Get OS ID from /etc/os-release
OS_ID=$(grep '^ID=' /etc/os-release | cut -d= -f2 | tr -d '"')

# Set custom PS1 for builduser
echo "export PS1='\\[\\e[36;1;2m\\]${OS_ID}@\\W\\[\\e[0m\\]\\\$ '" >> /home/builduser/.bashrc

# Change to workspace directory
cd /workspace

# Use gosu to properly switch user with TTY preservation
exec gosu builduser "$@"
```

### 3. Update Makefile

Add OS detection in the Makefile (around line 255):
```makefile
else ifeq ($(OS_ID),<os>)
    OS_DIR := <os>
    PACKAGE_TYPE := <deb|rpm>
    PACKAGE_BUILDER := <dpkg-deb|rpmbuild>
```

Add to architecture mapping (around line 297):
```makefile
else ifeq ($(OS_ID),<os>)
    PACKAGE_ARCH := $(DEB_ARCH)  # or $(RPM_ARCH)
```

Add Docker image configuration (around line 805):
```makefile
<os>_IMAGE = <base-image>
<os>_IMAGE_SOURCE = <Docker Hub|Other Registry>
```

Add to all relevant targets:
- `pull-<os>` target
- `build-<os>` target (uses pattern rule)
- `shell-<os>` target (uses pattern rule)
- `package-<os>` target
- Update `packages` target to include new OS
- Update help text

### 4. Create Package Templates

For DEB-based systems, create:
- `pkg/<os>/templates/control.template`
- `pkg/<os>/templates/postinst`
- `pkg/<os>/templates/prerm`
- `pkg/<os>/templates/conffiles`
- `pkg/<os>/templates/space-captain-server.service`

For RPM-based systems, create:
- `pkg/<os>/templates/space-captain.spec.template`
- `pkg/<os>/templates/space-captain-server.service`

### 5. Update Package Build Scripts

For DEB packages, the existing `scripts/build-deb-package.sh` should work with the OS_DIR parameter.

For RPM packages, the existing `scripts/build-rpm-package.sh` should work with the OS_DIR parameter.

### 6. Test the New OS Support

```bash
# Build Docker image
make build-<os>

# Enter build environment
make shell-<os>

# Inside container, test building
make clean-all
make
make tests
make run-tests

# Test packaging
make package

# From host, test full package build
make package-<os>
```

## Troubleshooting

### RPATH Errors
If you see RPATH errors during RPM packaging:
- Ensure `chrpath` is installed in the Docker image
- Verify the spec file includes `chrpath -d` command
- Check that RPATH stripping happens after binary installation

### Missing Dependencies
If packages fail to install due to missing libraries:
- Verify mbedTLS libraries are being copied to the package
- Check that the wrapper script sets `LD_LIBRARY_PATH` correctly
- Ensure library dependency exclusions are in place

### Docker Build Failures
If Docker images fail to build:
- Check base image availability
- Verify package manager commands for the OS
- Ensure all required tools are available in the base image

## Best Practices

1. **Always test in clean environment**: Use `make clean-all` before testing
2. **Verify RPATH is stripped**: Use `objdump -x <binary> | grep RPATH` on packaged binaries
3. **Test package installation**: Install and run the package in a clean container
4. **Follow OS conventions**: Each OS has its own packaging standards - follow them
5. **Document OS-specific quirks**: Add notes about any special handling required