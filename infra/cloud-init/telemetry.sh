#!/bin/bash
# Space Captain Telemetry initialization script

set -e

echo "Starting Space Captain Telemetry initialization at $(date)" | tee /var/log/space-captain-init.log

# Update all installed packages
echo "Updating all installed packages..." | tee -a /var/log/space-captain-init.log
sudo yum update -y

# Install Docker
echo "Installing Docker..." | tee -a /var/log/space-captain-init.log
sudo yum install -y docker
sudo systemctl enable docker
sudo systemctl start docker

# Wait for Docker to be ready
sleep 5

# Run Prometheus container
echo "Starting Prometheus..." | tee -a /var/log/space-captain-init.log
sudo docker run -d \
    --name prometheus \
    --restart always \
    -p 9090:9090 \
    -v /etc/prometheus:/etc/prometheus \
    prom/prometheus

# Create basic Prometheus config
sudo mkdir -p /etc/prometheus
cat > /tmp/prometheus.yml <<EOL
global:
  scrape_interval: 15s

scrape_configs:
  - job_name: 'space-captain'
    static_configs:
      - targets: ['localhost:9090']
EOL
sudo mv /tmp/prometheus.yml /etc/prometheus/

echo "Telemetry initialization complete at $(date)" | tee -a /var/log/space-captain-init.log