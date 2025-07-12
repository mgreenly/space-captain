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

variable "server_version" {
  description = "Version of the server package"
  type        = string
  default     = "0.1.1"
}

variable "server_release" {
  description = "Release number of the server package"
  type        = string
  default     = "1"
}

variable "server_rpm_filename" {
  description = "Filename of the server RPM package in S3 (leave empty to auto-generate)"
  type        = string
  default     = ""
}
