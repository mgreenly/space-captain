# Space Captain Development VM

This directory contains everything needed to run a QEMU/KVM virtual machine for Space Captain development.

## Prerequisites

Install required packages:
```bash
sudo apt-get install qemu-system-x86 qemu-utils genisoimage cloud-image-utils socat
```

## VM Specifications

- **OS**: Debian 12 (Bookworm) Cloud Image
- **Architecture**: x86_64
- **CPUs**: 4
- **Memory**: 4GB
- **Disk**: 20GB
- **Network**: User-mode networking with SSH port forwarding (localhost:2222)

## Quick Start

```bash
# Start the VM
make aarch64-start

# SSH into the VM
./vm/ssh-vm.sh

# Check VM status
make aarch64-status

# Stop the VM
make aarch64-stop

# Destroy the VM (removes disk and cloud-init seed)
make aarch64-destroy
```

## SSH Access

The VM is configured with SSH key authentication using the project's SSH key:
- **Username**: `debian`
- **SSH Key**: Uses the key at `.secrets/ssh/space-captain`
- **Port**: 2222

Connect using: `./vm/ssh-vm.sh`

Or manually: `ssh -i .secrets/ssh/space-captain -p 2222 debian@localhost`

## File Sharing

The project root directory is available inside the VM at `/workspace` using 9P filesystem (currently disabled, will be enabled in future).

## Installed Packages

The VM comes pre-installed with all development tools needed for Space Captain:
- build-essential
- git, vim, htop
- libmbedtls-dev
- clang-format
- gdb, valgrind
- make, cmake
- openssh-server

## Architecture Note

The VM was originally configured for ARM64 (aarch64) but has been switched to x86_64 for better performance on x86_64 host systems. The Makefile targets still use the "aarch64" prefix for backward compatibility.

## Troubleshooting

### SSH Connection Issues

If SSH fails to connect:
1. Check if the VM is still booting: `tail -f vm/space-captain-dev.serial`
2. Wait for the message "Space Captain Dev VM is ready!"
3. Ensure the SSH key exists at `.secrets/ssh/space-captain`

### Host Key Warnings

The `ssh-vm.sh` script is configured to not save host keys, so you won't get warnings when recreating VMs.

### Performance Issues

The VM uses KVM acceleration when available. If performance is poor, check:
```bash
# Verify KVM is available
ls /dev/kvm

# Check if your user can access KVM
groups | grep kvm
```

## Files

- `config.sh` - Central configuration for VM settings
- `cloud-init/` - Cloud-init configuration files
- `scripts/` - VM management scripts
- `images/` - VM disk images (created on first run)
- `ssh-vm.sh` - SSH helper script
- `*.serial` - Serial console output
- `*.monitor` - QEMU monitor socket
- `*.pid` - VM process ID

## Commands to Use the VM

1. **Start the VM**: `make aarch64-start`
2. **SSH into the VM**: `./vm/ssh-vm.sh`
3. **Run commands in the VM**: `./vm/ssh-vm.sh "ls -la"`
4. **Check VM status**: `make aarch64-status`
5. **Stop the VM**: `make aarch64-stop`
6. **Destroy and clean up**: `make aarch64-destroy`