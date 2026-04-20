#!/bin/bash

# Print enter on entrypoint bash script
echo "✅ Entering the docker entrypoint script"

# Check user
echo "👤 Current user: $(whoami)"

# Apply the correct permissions to the workspace directory
echo "🔐 Applying permissions to the: $WORKSPACE_DIR" 

# Set all files with the docker user as the owner
sudo chown --no-dereference -R $(whoami) $WORKSPACE_DIR

# Set executable permissions to all files within a relative path
sudo chmod -R +x ./tools/tasks

echo "📁 Linking opencode folder to user home directory"
# Link opencode mounted folder to user home directory
mkdir -p "$HOME/.local/share" \
&& ln -sfn /opt/opencode-host "$HOME/.local/share/opencode"

# Adding environment variables
gh_env_path="$HOME/.local/share/opencode/gh.env"
bashrc_path="$HOME/.bashrc"
gh_env_source_line='source "$HOME/.local/share/opencode/gh.env"'

if [[ -f "$gh_env_path" ]]; then
  if ! grep -Fqx "$gh_env_source_line" "$bashrc_path"; then
    printf '%s\n' "$gh_env_source_line" >> "$bashrc_path"
  fi

  # shellcheck disable=SC1090
  source "$gh_env_path"
else
  echo "❌ Missing GitHub environment file: $gh_env_path"
fi