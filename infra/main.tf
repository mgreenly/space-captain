terraform {
  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 5.0"
    }
  }
}

provider "aws" {
  region = var.aws_region
}

# SSH Key (permanent resource)
resource "aws_key_pair" "space_captain" {
  key_name   = "space-captain-key-${data.aws_caller_identity.current.account_id}"
  public_key = file("${path.module}/../.secrets/ssh/space-captain.pub")
}

# Data source needed by SSH key
data "aws_caller_identity" "current" {}

# Compute the RPM filename
locals {
  computed_rpm_filename = var.server_rpm_filename != "" ? var.server_rpm_filename : "space-captain-server-${var.server_version}-${var.server_release}.x86_64.rpm"
}

# Dynamic resources that can be torn down and recreated

# Server instance
resource "aws_instance" "server" {
  ami           = data.aws_ami.amazon_linux.id
  instance_type = "t3.nano"
  key_name      = aws_key_pair.space_captain.key_name
  
  vpc_security_group_ids = [aws_security_group.server.id]
  
  user_data = templatefile("${path.module}/cloud-init/server.sh.tpl", {
    rpm_filename = local.computed_rpm_filename
  })
  
  tags = {
    Name = "space-captain-server"
    Type = "server"
    "space-captain-server" = "true"
  }
}

# Client Auto Scaling Group
resource "aws_launch_template" "client" {
  name_prefix   = "space-captain-client-"
  image_id      = data.aws_ami.amazon_linux.id
  instance_type = "t3.nano"
  key_name      = aws_key_pair.space_captain.key_name
  
  vpc_security_group_ids = [aws_security_group.client.id]
  
  user_data = base64encode(file("${path.module}/cloud-init/client.sh"))
  
  tag_specifications {
    resource_type = "instance"
    tags = {
      Name = "space-captain-client"
      Type = "client"
      "space-captain-client" = "true"
    }
  }
}

resource "aws_autoscaling_group" "clients" {
  name_prefix        = "space-captain-clients-"
  vpc_zone_identifier = data.aws_subnets.default.ids
  min_size           = var.client_count
  max_size           = var.client_count
  desired_capacity   = var.client_count
  
  launch_template {
    id      = aws_launch_template.client.id
    version = "$Latest"
  }
}

# Telemetry instance
resource "aws_instance" "telemetry" {
  ami           = data.aws_ami.amazon_linux.id
  instance_type = "t3.nano"
  key_name      = aws_key_pair.space_captain.key_name
  
  vpc_security_group_ids = [aws_security_group.telemetry.id]
  
  user_data = file("${path.module}/cloud-init/telemetry.sh")
  
  tags = {
    Name = "space-captain-telemetry"
    Type = "telemetry"
    "space-captain-telemetry" = "true"
  }
}

# Data sources
data "aws_vpc" "default" {
  default = true
}

data "aws_subnets" "default" {
  filter {
    name   = "vpc-id"
    values = [data.aws_vpc.default.id]
  }
  
  filter {
    name   = "availability-zone"
    values = ["us-east-1a", "us-east-1b", "us-east-1d", "us-east-1e", "us-east-1f"]
  }
}

data "aws_ami" "amazon_linux" {
  most_recent = true
  owners      = ["amazon"]
  
  filter {
    name   = "name"
    values = ["al2023-ami-*-x86_64"]
  }
  
  filter {
    name   = "virtualization-type"
    values = ["hvm"]
  }
}

# Security Groups
resource "aws_security_group" "server" {
  name_prefix = "space-captain-server-"
  description = "Security group for Space Captain server"
  vpc_id      = data.aws_vpc.default.id
  
  ingress {
    from_port   = 22
    to_port     = 22
    protocol    = "tcp"
    cidr_blocks = ["216.173.146.119/32"]
    description = "SSH access from my IP only"
  }
  
  ingress {
    from_port       = 4242
    to_port         = 4242
    protocol        = "tcp"
    security_groups = [aws_security_group.client.id]
  }
  
  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
}

resource "aws_security_group" "client" {
  name_prefix = "space-captain-client-"
  description = "Security group for Space Captain clients"
  vpc_id      = data.aws_vpc.default.id
  
  ingress {
    from_port   = 22
    to_port     = 22
    protocol    = "tcp"
    cidr_blocks = ["216.173.146.119/32"]
    description = "SSH access from my IP only"
  }
  
  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
}

resource "aws_security_group" "telemetry" {
  name_prefix = "space-captain-telemetry-"
  description = "Security group for Space Captain telemetry"
  vpc_id      = data.aws_vpc.default.id
  
  ingress {
    from_port   = 22
    to_port     = 22
    protocol    = "tcp"
    cidr_blocks = ["216.173.146.119/32"]
    description = "SSH access from my IP only"
  }
  
  ingress {
    from_port       = 9090
    to_port         = 9090
    protocol        = "tcp"
    security_groups = [aws_security_group.server.id, aws_security_group.client.id]
  }
  
  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
}
