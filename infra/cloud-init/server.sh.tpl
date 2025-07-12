#!/bin/bash
# Space Captain Server initialization script

set -e

echo "Starting Space Captain Server initialization at $(date)" | tee /var/log/space-captain-init.log

# Update all installed packages
echo "Updating all installed packages..." | tee -a /var/log/space-captain-init.log
sudo yum update -y

# Download and install server package
echo "Downloading server package: ${rpm_filename}" | tee -a /var/log/space-captain-init.log
curl -o /tmp/space-captain-server.rpm https://s3.us-east-1.amazonaws.com/space-captain.metaspot.org/${rpm_filename}

echo "Installing server package..." | tee -a /var/log/space-captain-init.log
sudo rpm -i /tmp/space-captain-server.rpm

# Configure and start server
# echo "Starting Space Captain Server..." | tee -a /var/log/space-captain-init.log
# sudo systemctl enable space-captain-server
# sudo systemctl start space-captain-server

echo "Server initialization complete at $(date)" | tee -a /var/log/space-captain-init.log