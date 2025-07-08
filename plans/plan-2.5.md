# Plan 2.5: Debian Packaging and Release Automation

**Status**: Planning  
**Parent Document**: [Space Captain v0.1.0 PRD](../prd/version-0.1.0.md)  
**Scope**: Infrastructure and release engineering for v0.1.0

This plan outlines the implementation of Debian packaging for the Space Captain server and automated release workflows for multiple architectures. It extends the core v0.1.0 release with professional packaging and distribution capabilities.

---

## Phase 1: Debian Package Creation (Week 1)

### Goal

Create a proper Debian 12 package (.deb) that includes only the Space Captain server binary with appropriate system integration.

### Tasks

#### 1.1 Package Structure Setup

Create the foundational Debian packaging structure following official standards:

1. **Directory Structure**
   - Create `debian/` directory with all required packaging files
   - Follow Debian packaging standards and best practices

2. **Package Metadata** (`debian/control`)
   - Package name: `space-captain-server`
   - Version: Derived from current git tag
   - Architecture: `amd64` initially, later `arm64`
   - Dependencies: `libc6`, `libmbedtls14`
   - Description: "Space Captain MMO Server - High-performance space combat server"

#### 1.2 Build Configuration

Configure the build process to integrate with the existing Makefile:

1. **Build Rules** (`debian/rules`)
   - Use existing Makefile with `make release` target
   - Configure proper optimization flags for production

2. **Installation Paths**
   - Binary: `/usr/bin/space-captain-server`
   - Systemd service: `/lib/systemd/system/space-captain-server.service`

#### 1.3 System Integration

Create comprehensive system integration for production deployment:

1. **Systemd Service Configuration**
   - User/Group: `space-captain` (unprivileged system user)
   - Restart policy: `Restart=on-failure` with `RestartSec=5`
   - Logging: Use systemd journal (stdout/stderr)
   - Resource limits:
     - `LimitNOFILE=65536` for high connection counts
     - `PrivateTmp=yes` for security isolation

2. **Package Scripts**
   - **postinst**: Create system user/group and enable service
     ```bash
     # Create system group if it doesn't exist
     if ! getent group space-captain >/dev/null; then
         addgroup --system space-captain
     fi
     # Create system user if it doesn't exist
     if ! getent passwd space-captain >/dev/null; then
         adduser --system --no-create-home --ingroup space-captain \
                 --shell /usr/sbin/nologin space-captain
     fi
     ```
   - **prerm**: Stop and disable service on removal

#### 1.4 Docker Build Environment

Create an isolated build environment for consistent package creation:

1. **Build Container** (`docker/debian-build.dockerfile`)
   - Base image: `debian:12-slim`
   - Build dependencies: `build-essential`, `debhelper`, `libmbedtls-dev`

2. **Build Script**
   - Mount project directory into container
   - Build .deb package in isolated environment
   - Copy resulting package to host `dist/` directory

#### 1.5 Monitoring Integration

Integrate comprehensive monitoring capabilities into the package:

1. **Prometheus Metrics Configuration**
   - Configuration file: `/etc/space-captain/prometheus.conf`
   - Metrics endpoint: Port 9100, path `/metrics`
   - Available metrics:
     - `space_captain_connections_total` - Total connection count
     - `space_captain_active_connections` - Current active connections
     - `space_captain_messages_processed_total` - Messages by type
     - `space_captain_tick_duration_seconds` - Game tick processing time
     - `space_captain_memory_usage_bytes` - Process memory usage
     - `space_captain_cpu_usage_percent` - CPU utilization

2. **Grafana Dashboard Template**
   - Location: `/usr/share/space-captain-server/monitoring/grafana-dashboard.json`
   - Dashboard panels:
     - Connection metrics (current, total, rate)
     - Performance metrics (tick rate, latency)
     - Resource usage (CPU, memory, network)
     - Error rates and alerts
     - Message type distribution

3. **Monitoring Documentation**
   - Prometheus scrape configuration examples
   - Alert rules for common issues:
     - High connection count (>80% of limit)
     - Tick rate degradation (<55 ticks/second)
     - Memory usage growth
     - Connection error rates
   - Performance tuning guidelines
   - Capacity planning recommendations

4. **Container-Friendly Monitoring**
   - Environment variable configuration
   - Health check endpoint at `/health`
   - Kubernetes service monitor examples
   - Docker Compose monitoring stack examples

#### 1.6 Makefile Integration

Add package building targets to the main Makefile:

1. **Build Targets**
   - `make deb` - Build Debian package using Docker
   - `make deb-test` - Build and test package installation
   - `make deb-local` - Build without Docker (for developers)
   - `make clean-deb` - Remove packaging artifacts

2. **Testing Integration**
   - Automated package installation verification
   - Service start/stop testing
   - Clean uninstall validation

#### 1.7 Local Developer Experience

Support developers building packages on their local systems:

1. **Non-Docker Building**
   - `make deb-local` target for local builds
   - Prerequisite checking for build tools
   - Clear error messages for missing dependencies

2. **Manual Build Instructions**
   ```bash
   # Install build dependencies
   sudo apt-get install build-essential debhelper libmbedtls-dev
   
   # Build the package
   dpkg-buildpackage -us -uc -b
   
   # Install the package
   sudo dpkg -i ../space-captain-server_*.deb
   
   # Verify installation
   systemctl status space-captain-server
   ```

3. **Development Workflow**
   - Quick iteration with `make deb-dev`
   - Debug symbol preservation
   - Lintian check skipping for speed

4. **Troubleshooting Guide**
   - Common build errors and solutions
   - Debug techniques for build failures
   - Package content verification commands
   - Systemd service debugging

### Phase 1 Deliverables

- Complete `debian/` directory with all packaging files
- Working `make deb` target in the main Makefile
- Functional .deb package that installs and runs on Debian 12
- Comprehensive documentation for package building

---

## Phase 2: GitHub Release Automation (Week 2)

### Goal

Create GitHub Actions workflows that automatically build and publish release artifacts for both x86_64 and AArch64 architectures when a new tag is pushed.

### Tasks

#### 2.1 Build Matrix Setup

Configure multi-architecture build support:

1. **Workflow Configuration** (`.github/workflows/release.yml`)
   - Build matrix for x86_64 (amd64) and AArch64 (arm64)
   - GitHub-hosted runners or QEMU emulation
   - Parallel builds for efficiency

2. **Reproducible Build Environment**
   - Pin Docker images to specific digests
   - Lock tool versions (debhelper, mbedtls, compiler)
   - Create `versions.lock` file
   - Monthly security update review process

#### 2.2 Build and Verification Pipeline

Implement comprehensive build and testing pipeline:

1. **Build Process** (per architecture)
   - Build Debian package (.deb)
   - Run package installation tests
   - Build standalone binary (tarball)
   - Generate SHA256 checksums

2. **Artifact Naming**
   - `space-captain-server_0.1.0_amd64.deb`
   - `space-captain-server_0.1.0_arm64.deb`
   - `space-captain-server-0.1.0-linux-amd64.tar.gz`
   - `space-captain-server-0.1.0-linux-arm64.tar.gz`

3. **Build Caching Strategy**
   - Docker layer caching with buildx
   - ccache for C compilation (500MB limit)
   - Dependency caching for apt packages
   - Weekly cache warmup job

#### 2.3 Cross-Compilation Strategy

Support for ARM64 architecture builds:

1. **Build Options**
   - Native ARM runners (preferred if available)
   - Cross-compilation toolchain
   - QEMU emulation fallback

2. **Dependency Management**
   - Proper mbedTLS handling for each architecture
   - Library compatibility verification

#### 2.4 Release Creation

Automated draft release generation:

1. **Release Triggers**
   - Activate on version tags (e.g., `v0.1.0`)
   - Create draft GitHub release

2. **Release Contents**
   - Auto-generated changelog from commits
   - All built artifacts attached
   - Installation instructions
   - SHA256 checksums for verification

3. **Manual Review Process**
   - Releases created as drafts
   - Maintainer reviews and edits notes
   - Manual publishing when ready

4. **Artifact Management**
   - 30-day retention for build artifacts
   - 7-day retention for failed build logs
   - Weekly cleanup workflow
   - Size monitoring and alerts

#### 2.5 Security Integration

Comprehensive security scanning and validation:

1. **Vulnerability Scanning**
   - Trivy scanner integration
   - Scan Docker images and binaries
   - Fail on CRITICAL vulnerabilities
   - Manual approval for HIGH vulnerabilities

2. **SBOM Generation**
   - Software Bill of Materials with Syft
   - SPDX and CycloneDX formats
   - Attach to release artifacts

3. **Dependency Checking**
   - C dependency scanning with osv-scanner
   - mbedTLS CVE verification
   - System library validation
   - Security report generation

4. **Continuous Monitoring**
   - Daily scans on latest release
   - GitHub issue creation for vulnerabilities
   - Security advisory maintenance
   - Time-to-patch tracking

#### 2.6 Post-Release Validation

Automated verification of published releases:

1. **Validation Workflow**
   - Download all release artifacts
   - Test package installation on fresh systems
   - Verify service functionality
   - Performance baseline testing

2. **Validation Reports**
   - Success/failure status per artifact
   - Performance metrics
   - Anomaly detection
   - Results posted to release comments

3. **Multi-Distribution Testing**
   - Debian 12 (primary target)
   - Ubuntu 22.04 LTS
   - Ubuntu 24.04 LTS
   - Linux Mint Debian Edition
   - Compatibility documentation

### Phase 2 Deliverables

- Complete GitHub Actions workflow
- Automated multi-architecture builds
- Draft release creation with artifacts
- Comprehensive release documentation

---

## Success Criteria

### 1. Debian Package Quality
- Clean installation on fresh Debian 12 systems
- Systemd service runs without manual intervention
- Adherence to Debian packaging best practices
- Complete removal on uninstall

### 2. Release Automation
- Automatic builds triggered by version tags
- Successful multi-architecture builds
- Passing verification tests in CI
- Proper artifact naming and versioning
- Draft releases with editable notes

### 3. Cross-Platform Support
- ARM64 packages functional on target hardware
- Binary compatibility verification
- Consistent performance across architectures

---

## Technical Considerations

### Debian Packaging
- debhelper compat level 13 (Debian 12 standard)
- Proper shared library dependency management
- lintian validation for package quality

### GitHub Actions
- Build dependency caching
- Secret management for future signing
- Build time optimization
- Status badge integration

### Security
- Future GPG signing support
- SHA256 checksums for all artifacts
- Documented verification procedures
- Reproducible build considerations

---

## Future Enhancements (Post-v1)

- RPM packages for Red Hat distributions
- Docker/OCI container images
- Homebrew formula for macOS
- Snap or Flatpak packages
- APT repository hosting
- Automatic version bumping