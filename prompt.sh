# --- Custom Dynamic Prompt ---
# Sets a prompt that displays OS ID and architecture.

# Get the OS identifier from /etc/os-release.
# We source the file in a subshell to avoid polluting the current shell's environment.
if [ -f /etc/os-release ]; then
    os_id=$(. /etc/os-release && echo "$ID")
else
    os_id="linux" # Fallback value if the file doesn't exist.
fi

# Get the machine architecture from uname.
arch=$(uname -m)

# Set the PS1 prompt string using the variables.
# Format: [os_id:arch]@WorkingDirectory$
# Color: Bold Yellow
PS1="\[\e[33;1m\][${os_id}:${arch}]@\W\\ \$ \[\e[0m\]"

# Clean up the temporary variables as they are no longer needed.
unset os_id arch

