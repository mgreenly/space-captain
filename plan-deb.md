# Debian Package Creation Plan

## Objective
Create a 'make package' target that depends on 'make release' and detects the OS to create a *.deb file that is named `space-captain-server-<version>-<architecture>.deb`

## Requirements

### Package Contents
- The server binary
- Minimal files required by Debian standards
- Systemd service file to run the server service

### Installation Behavior
- Install the versioned file
- Create and forcefully create an unversioned symlink to it

### Directory Structure
- Create a `/pkg/deb` folder that holds the templates for these files

## Implementation Steps

### 1. Create Package Directory Structure
```
pkg/
└── deb/
    ├── control.template
    ├── postinst
    ├── space-captain-server.service
    └── Makefile.deb
```

### 2. Makefile Target
- Add 'make package' target that depends on 'make release'
- Detect OS (Linux distribution)
- Detect architecture (amd64, arm64, etc.)
- Extract version from VERSION file
- Build .deb package with proper naming

### 3. Debian Control File Template
- Package name: space-captain-server
- Version: from VERSION file
- Architecture: detected
- Maintainer information
- Description
- Dependencies (if any)

### 4. Post-Install Script
- Create versioned installation in /usr/bin/
- Create unversioned symlink pointing to versioned binary
- Enable and start systemd service

### 5. Systemd Service File
- Service name: space-captain-server
- ExecStart: /usr/bin/space-captain-server
- Restart policy
- User/Group settings
- Working directory
- Environment variables if needed

### 6. Package Building Process
- Create temporary build directory
- Copy release binary
- Generate control file from template
- Create DEBIAN directory structure
- Build .deb using dpkg-deb
- Output to project root with proper naming