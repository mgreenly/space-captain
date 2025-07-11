#!/bin/bash
# Sync version and release from project files to terraform.tfvars and upload RPM to S3

set -e

VERSION=$(cat ../.VERSION)
RELEASE=$(cat ../.RELEASE)
RPM_FILENAME="space-captain-server-${VERSION}-${RELEASE}.amzn2023.x86_64.rpm"
RPM_PATH="../pkg/out/${RPM_FILENAME}"

echo "Syncing version $VERSION-$RELEASE to terraform.tfvars"

# Create or update terraform.tfvars
cat > terraform.tfvars <<EOF
# Auto-generated from .VERSION and .RELEASE files
server_version = "$VERSION"
server_release = "$RELEASE"
EOF

echo "terraform.tfvars updated with:"
echo "  server_version = $VERSION"
echo "  server_release = $RELEASE"
echo ""
echo "RPM filename: ${RPM_FILENAME}"

# Check if RPM exists
if [ ! -f "$RPM_PATH" ]; then
    echo ""
    echo "ERROR: RPM file not found at $RPM_PATH"
    echo "Please build the RPM first with: make package-amazon"
    exit 1
fi

# Upload to S3
echo ""
echo "Uploading RPM to S3..."
aws s3 cp "$RPM_PATH" "s3://space-captain.metaspot.org/${RPM_FILENAME}"

if [ $? -eq 0 ]; then
    echo "Successfully uploaded to: https://s3.us-east-1.amazonaws.com/space-captain.metaspot.org/${RPM_FILENAME}"
else
    echo "ERROR: Failed to upload RPM to S3"
    exit 1
fi