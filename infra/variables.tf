variable "aws_region" {
  description = "AWS region to deploy resources"
  type        = string
  default     = "us-east-1"
}

variable "client_count" {
  description = "Number of client instances to create"
  type        = number
  default     = 3
}

