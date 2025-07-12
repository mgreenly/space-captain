# Infrastructure Management Strategy

## Overview

This directory uses a two-file Terraform strategy to manage AWS resources:
- **dynamic.tf** - Contains all actively managed resources
- **permanent.tf.bak** - Contains resources that should persist (not tracked by Terraform state)

## Quick Start

```bash
# Build the RPM package
cd .. && make package-amazon && cd infra

# Sync version and upload RPM to S3
./sync-version.sh

# Deploy infrastructure
terraform apply
```

## Resource Lifecycle

### Creating New Resources
1. Always create new resources in `dynamic.tf`
2. Test and validate the resources work as expected
3. Apply changes with `terraform apply`

### Making Resources Permanent
When you want a resource to persist through `terraform destroy`:

1. Remove the resource from Terraform state:
   ```bash
   terraform state rm <resource_type>.<resource_name>
   ```

2. Copy the resource definition from `dynamic.tf` to `permanent.tf.bak`

3. Remove the resource definition from `dynamic.tf`

4. Apply changes to ensure consistency:
   ```bash
   terraform apply
   ```

### Destroying Dynamic Infrastructure
Simply run:
```bash
terraform destroy
```

This will only destroy resources tracked in the current state (from dynamic.tf).
Resources in permanent.tf.bak remain untouched since they're not in the state.

### Re-importing Permanent Resources
If you need to manage a permanent resource again:

1. Copy the resource definition from `permanent.tf.bak` to `dynamic.tf`

2. Import the existing resource into state:
   ```bash
   terraform import <resource_type>.<resource_name> <resource_id>
   ```

3. Verify with:
   ```bash
   terraform plan
   ```

## Current Permanent Resources

The following resources have been removed from state and exist in permanent.tf.bak:
- **aws_s3_bucket.space_captain** - Public S3 bucket for artifacts
- **aws_s3_bucket_public_access_block.space_captain** - Bucket access configuration
- **aws_s3_bucket_policy.space_captain** - Bucket policy for public read access

## Cloud-Init Scripts

The infrastructure uses cloud-init scripts to automatically configure instances on startup:

- **Server** (`cloud-init/server.sh.tpl`): Downloads and installs the server RPM from S3
- **Client** (`cloud-init/client.sh`): Finds server IP and configures client connection
- **Telemetry** (`cloud-init/telemetry.sh`): Installs Docker and runs Prometheus

### Customizing the Server RPM

The `sync-version.sh` script automates version management:
- Reads version from `../.VERSION` and release from `../.RELEASE`
- Updates `terraform.tfvars` with the correct values
- Uploads the RPM to S3 bucket

```bash
# Recommended: Use sync script to automate version management
./sync-version.sh

# Manual override: Use specific version and release
terraform apply -var="server_version=0.2.0" -var="server_release=1"

# Manual override: Use specific RPM filename
terraform apply -var="server_rpm_filename=space-captain-server-0.2.0-1.amzn2023.x86_64.rpm"
```

The RPM filename follows the pattern: `space-captain-server-<version>-<release>.amzn2023.x86_64.rpm`

### Checking Cloud-Init Status

To verify cloud-init ran successfully on an instance:

```bash
# SSH to instance, then:
sudo cat /var/log/cloud-init-output.log    # Full cloud-init output
cat /var/log/space-captain-init.log        # Our custom init log
```

## Example Workflow

```bash
# Create infrastructure
terraform apply

# Make the telemetry instance permanent
terraform state rm aws_instance.telemetry
# Then manually move the resource definition to permanent.tf.bak

# Destroy all dynamic resources (telemetry persists)
terraform destroy

# Later, re-import telemetry if needed
terraform import aws_instance.telemetry i-0123456789abcdef0
```
