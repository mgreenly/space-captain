# Space Captain Implementation Tasks

## Phase 1: Build System Updates (Foundation)

### 1.1 Update Makefile for Release Targets
- [ ] Add `release` target that detects OS via `/etc/os-release`
- [ ] Add `release-debian` target for native Debian builds
- [ ] Add `release-al2023` target (placeholder for Docker builds)
- [ ] Verify: `make release` on Debian calls `release-debian`
- [ ] Verify: Build outputs to `bin/` directory

### 1.2 Create Docker Build Environment
- [ ] Create `docker/al2023-builder/Dockerfile` with:
  - Base: `amazonlinux:2023`
  - Install: gcc, make, git, mbedtls-devel
  - Install: ruby, gem install fpm
  - Set working directory to `/workspace`
- [ ] Add Makefile target to build Docker image: `docker-build-image`
- [ ] Update `release-al2023` to use Docker container
- [ ] Verify: `make release-al2023` builds binaries in container
- [ ] Verify: Binaries are copied back to host `bin/` directory

## Phase 2: Packaging System

### 2.1 Create Systemd Service Files
- [ ] Create `systemd/space-captain-server.service`
  - ExecStart=/usr/local/bin/server
  - Restart=always
  - StandardOutput=journal
  - StandardError=journal
- [ ] Create `systemd/space-captain-client.service`
  - ExecStart=/usr/local/bin/client
  - Environment for server discovery
- [ ] Verify: Service files have correct syntax

### 2.2 Implement RPM Building
- [ ] Add `build-rpm` target to Makefile
- [ ] Create temporary staging directories in target
- [ ] Use Docker to run fpm for server RPM
- [ ] Use Docker to run fpm for client RPM
- [ ] Output RPMs to `dist/` directory
- [ ] Verify: RPMs contain correct files and paths
- [ ] Verify: RPM metadata (version, architecture) is correct

## Phase 3: GitHub Release Integration

### 3.1 Version Management
- [ ] Add VERSION file or use existing version system
- [ ] Update Makefile to read version for RPM builds
- [ ] Add `bump-version` target if not exists

### 3.2 GitHub Release Publishing
- [ ] Add `publish-release` target using `gh` CLI
- [ ] Script to create release with version tag
- [ ] Script to upload RPMs as release assets
- [ ] Verify: Manual trigger creates GitHub release
- [ ] Verify: RPMs are downloadable from release

## Phase 4: AWS Infrastructure Foundation

### 4.1 Terraform Setup
- [ ] Create `terraform/` directory structure
- [ ] Create `terraform/backend.tf` with S3 configuration
- [ ] Create `terraform/variables.tf` for configuration
- [ ] Create `terraform/vpc.tf` for networking
- [ ] Add `infra-init` Makefile target
- [ ] Verify: Terraform initializes with S3 backend

### 4.2 IAM Configuration
- [ ] Create `terraform/iam.tf` with:
  - Instance profile for EC2 instances
  - Role with EC2 and CloudWatch permissions
  - Policy attachments
- [ ] Verify: IAM resources created correctly

## Phase 5: EC2 Infrastructure

### 5.1 Security Groups
- [ ] Create security group for server (port 2000)
- [ ] Create security group for clients
- [ ] Configure ingress/egress rules
- [ ] Verify: Security groups allow required traffic

### 5.2 Server Instance Configuration
- [ ] Create `terraform/server.tf` with:
  - EC2 instance configuration
  - User data script for cloud-init
  - IAM instance profile attachment
  - Tagging configuration
- [ ] Create cloud-init script that:
  - Downloads RPM from GitHub
  - Installs RPM
  - Starts service
  - Tags instance with Role=space-captain-server

### 5.3 Client Auto-Scaling Group
- [ ] Create `terraform/clients.tf` with:
  - Launch template
  - Auto-scaling group
  - Cloud-init for client setup
- [ ] Configure service discovery in cloud-init
- [ ] Verify: Clients can find server via tags

## Phase 6: CloudWatch Integration

### 6.1 CloudWatch Agent Setup
- [ ] Add CloudWatch Agent installation to cloud-init
- [ ] Create agent configuration for log streaming
- [ ] Configure log groups in Terraform
- [ ] Verify: Logs appear in CloudWatch

### 6.2 Status and Monitoring Scripts
- [ ] Create `scripts/status.sh` to query CloudWatch
- [ ] Create `scripts/logs.sh` to stream logs
- [ ] Add corresponding Makefile targets
- [ ] Verify: Can view status without SSH

## Phase 7: Operational Scripts

### 7.1 Deployment Scripts
- [ ] Create `scripts/deploy.sh` for full deployment
- [ ] Create scripts for start/stop operations
- [ ] Use AWS CLI/API for instance control
- [ ] Add Makefile targets for all operations

### 7.2 Service Discovery Implementation
- [ ] Update server code to tag instance on startup
- [ ] Update client code to query AWS API for server
- [ ] Test end-to-end connectivity
- [ ] Verify: No hardcoded IPs needed

## Phase 8: Testing and Documentation

### 8.1 Integration Testing
- [ ] Test full deployment flow
- [ ] Test service discovery
- [ ] Test CloudWatch logging
- [ ] Test start/stop operations
- [ ] Document any issues found

### 8.2 Documentation Updates
- [ ] Update README with deployment instructions
- [ ] Document all Makefile targets
- [ ] Create troubleshooting guide
- [ ] Add architecture diagram

## Dependencies and Order

1. **Phase 1 must complete first** - Build system is foundation
2. **Phase 2 depends on Phase 1** - Need binaries to package
3. **Phase 3 depends on Phase 2** - Need RPMs to publish
4. **Phase 4 can start anytime** - Terraform setup is independent
5. **Phase 5 depends on Phases 3 & 4** - Needs RPMs and Terraform
6. **Phase 6 depends on Phase 5** - Needs instances for CloudWatch
7. **Phase 7 depends on Phases 5 & 6** - Needs full infrastructure
8. **Phase 8 is final** - Testing requires everything complete

## Verification Checklist

Each task includes verification steps to ensure correctness before moving forward. This prevents cascading issues and ensures each component works independently.