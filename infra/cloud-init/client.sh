#!/bin/bash
# Space Captain Client initialization script

set -e

echo "Starting Space Captain Client initialization at $(date)" | tee /var/log/space-captain-init.log

# Update all installed packages
echo "Updating all installed packages..." | tee -a /var/log/space-captain-init.log
sudo yum update -y

# Install useful system packages
echo "Installing system packages..." | tee -a /var/log/space-captain-init.log
sudo yum install -y \
    htop \
    tmux \
    vim \
    git \
    jq \
    strace \
    tcpdump \
    netcat \
    bind-utils \
    sysstat \
    aws-cli

echo "Client initialization complete at $(date)" | tee -a /var/log/space-captain-init.log