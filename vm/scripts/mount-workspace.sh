#!/bin/bash
# Helper script to mount workspace in the VM
# Can be run manually if the workspace mount fails during boot

set -e

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "Please run as root (use sudo)"
    exit 1
fi

# Check if workspace is already mounted
if mount | grep -q "/workspace"; then
    echo "Workspace is already mounted"
    exit 0
fi

# Create mount point if it doesn't exist
mkdir -p /workspace

# Function to try mounting with 9p
try_9p_mount() {
    echo "Attempting to mount workspace using 9p..."
    
    # Check if 9p modules are available
    if ! lsmod | grep -q 9p; then
        echo "Loading 9p modules..."
        if modprobe -n 9p 2>/dev/null; then
            modprobe 9p
            modprobe 9pnet
            modprobe 9pnet_virtio
            
            # Add to modules for persistence
            if ! grep -q "^9p$" /etc/modules; then
                echo -e "9p\n9pnet\n9pnet_virtio" >> /etc/modules
            fi
        else
            echo "9p modules not available in this kernel"
            return 1
        fi
    fi
    
    # Try to mount with different options
    echo "Mounting workspace..."
    
    # Try with optimized settings first
    if mount -t 9p -o trans=virtio,version=9p2000.L,rw,cache=loose,msize=512000 workspace /workspace 2>/dev/null; then
        echo "Successfully mounted with optimized settings"
        return 0
    fi
    
    # Try with basic settings
    if mount -t 9p -o trans=virtio,version=9p2000.L,rw workspace /workspace 2>/dev/null; then
        echo "Successfully mounted with basic settings"
        return 0
    fi
    
    # Try with legacy version
    if mount -t 9p -o trans=virtio,rw workspace /workspace 2>/dev/null; then
        echo "Successfully mounted with legacy settings"
        return 0
    fi
    
    echo "Failed to mount workspace with 9p"
    return 1
}

# Function to set up SSHFS as fallback
setup_sshfs() {
    echo "Setting up SSHFS as fallback..."
    
    # Install sshfs if not available
    if ! command -v sshfs &> /dev/null; then
        echo "Installing sshfs..."
        apt-get update
        apt-get install -y sshfs
    fi
    
    echo ""
    echo "SSHFS is available as a fallback option."
    echo "To mount the workspace using SSHFS from the host, run:"
    echo ""
    echo "  sshfs -o allow_other,default_permissions user@10.0.2.2:/path/to/space-captain /workspace"
    echo ""
    echo "Replace 'user' with your host username and adjust the path as needed."
}

# Main logic
echo "Space Captain Workspace Mount Helper"
echo "===================================="

if try_9p_mount; then
    echo ""
    echo "Workspace mounted successfully at /workspace"
    
    # Add to fstab if not already there
    if ! grep -q "^workspace" /etc/fstab; then
        echo "workspace /workspace 9p trans=virtio,version=9p2000.L,rw,cache=loose,msize=512000,_netdev 0 0" >> /etc/fstab
        echo "Added to /etc/fstab for persistence"
    fi
else
    echo ""
    echo "ERROR: 9p mount failed"
    echo ""
    echo "This is likely because you're using a cloud kernel that doesn't include 9p modules."
    echo ""
    echo "Solutions:"
    echo "1. Install the full kernel: apt-get install linux-image-amd64"
    echo "2. Use SSHFS as an alternative (see below)"
    echo ""
    
    setup_sshfs
    
    exit 1
fi

# Set permissions for debian user
chown -R debian:debian /workspace 2>/dev/null || true

echo ""
echo "Done!"