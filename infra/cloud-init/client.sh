#!/bin/bash
# Space Captain Client initialization script

set -e

echo "Starting Space Captain Client initialization at $(date)" | tee /var/log/space-captain-init.log

# Update all installed packages
echo "Updating all installed packages..." | tee -a /var/log/space-captain-init.log
sudo yum update -y

# Install AWS CLI if not present
if ! command -v aws &> /dev/null; then
    echo "Installing AWS CLI..." | tee -a /var/log/space-captain-init.log
    sudo yum install -y aws-cli
fi

# Get server IP from AWS
echo "Finding server IP..." | tee -a /var/log/space-captain-init.log
SERVER_IP=$(aws ec2 describe-instances --region us-east-1 \
    --filters "Name=tag:Name,Values=space-captain-server" "Name=instance-state-name,Values=running" \
    --query 'Reservations[0].Instances[0].PrivateIpAddress' \
    --output text)

echo "Server IP: $SERVER_IP" | tee -a /var/log/space-captain-init.log

# Download client binary
# echo "Downloading client binary..." | tee -a /var/log/space-captain-init.log
# curl -o /usr/local/bin/space-captain-client https://s3.us-east-1.amazonaws.com/space-captain.metaspot.org/space-captain-client
# chmod +x /usr/local/bin/space-captain-client

# Create systemd service for client
# cat > /etc/systemd/system/space-captain-client.service <<EOL
# [Unit]
# Description=Space Captain Client
# After=network.target
# 
# [Service]
# Type=simple
# ExecStart=/usr/local/bin/space-captain-client --server=${SERVER_IP}:4242
# Restart=always
# 
# [Install]
# WantedBy=multi-user.target
# EOL

# Start client service
# echo "Starting Space Captain Client..." | tee -a /var/log/space-captain-init.log
# sudo systemctl enable space-captain-client
# sudo systemctl start space-captain-client

echo "Client initialization complete at $(date)" | tee -a /var/log/space-captain-init.log