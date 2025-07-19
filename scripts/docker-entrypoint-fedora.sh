#!/bin/bash
# Docker entrypoint script for Fedora builder container
# Creates a user with matching UID/GID to avoid permission issues

USER_ID=${USER_ID:-1000}
GROUP_ID=${GROUP_ID:-1000}

# Create group and user with matching IDs (silently)
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