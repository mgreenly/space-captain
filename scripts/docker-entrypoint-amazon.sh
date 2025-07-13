#!/bin/bash
# Docker entrypoint script for Amazon Linux builder container
# Creates a user with matching UID/GID to avoid permission issues

USER_ID=${USER_ID:-1000}
GROUP_ID=${GROUP_ID:-1000}

echo "Creating user with UID=$USER_ID and GID=$GROUP_ID"

# Create group and user with matching IDs
groupadd -g $GROUP_ID -o builduser 2>/dev/null || true
useradd -u $USER_ID -g $GROUP_ID -o -m -s /bin/bash builduser 2>/dev/null || true

# Export LD_LIBRARY_PATH for the user
export LD_LIBRARY_PATH=/workspace/.local/amazon/lib

# Run command as builduser
exec runuser -u builduser -- bash -c "cd /workspace && LD_LIBRARY_PATH=/workspace/.local/amazon/lib $*"