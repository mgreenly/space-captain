output "server_public_ip" {
  description = "Public IP address of the server"
  value       = aws_instance.server.public_ip
}

output "server_private_ip" {
  description = "Private IP address of the server"
  value       = aws_instance.server.private_ip
}

output "telemetry_public_ip" {
  description = "Public IP address of the telemetry instance"
  value       = aws_instance.telemetry.public_ip
}

output "client_asg_name" {
  description = "Name of the client Auto Scaling Group"
  value       = aws_autoscaling_group.clients.name
}
