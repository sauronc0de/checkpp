#!/bin/bash

# Log entry into the container entrypoint script
echo "✅ Entering the docker entrypoint script"

# Print the current user
echo "👤 Current user: $(whoami)"

# Show the workspace directory where permissions will be applied
echo "🔐 Applying permissions to: $WORKSPACE_DIR"

# Set the current user as the owner of all files under the workspace directory
sudo chown --no-dereference -R "$(whoami)" "$WORKSPACE_DIR"

# Make scripts executable
chmod -R a+x "$WORKSPACE_DIR/scripts" "$WORKSPACE_DIR/tools/scripts"

# Make commands visible from anywhere
export PATH="$PATH:$WORKSPACE_DIR/scripts:$WORKSPACE_DIR/tools/scripts"

echo "📁 Linking opencode folder to user home directory"

# Link the mounted opencode folder into the user's local share directory
mkdir -p "$HOME/.local/share" \
&& ln -sfn /opt/opencode-host "$HOME/.local/share/opencode"

# Persist environment setup for future interactive bash sessions
bashrc_path="$HOME/.bashrc"
gh_env_path="$HOME/.local/share/opencode/gh.env"
gh_env_source_line='source "$HOME/.local/share/opencode/gh.env"'
path_line='export PATH="$PATH:$WORKSPACE_DIR/scripts"'

# Ensure the external environment file is sourced from .bashrc if it exists
if [[ -f "$gh_env_path" ]]; then
  if ! grep -Fqx "$gh_env_source_line" "$bashrc_path"; then
    printf '%s\n' "$gh_env_source_line" >> "$bashrc_path"
  fi

  # Load environment variables for the current shell session now
  # shellcheck disable=SC1090
  source "$gh_env_path"
else
  echo "❌ Missing GitHub environment file: $gh_env_path"
fi

# Add tool directories to PATH in .bashrc AFTER gh.env
# This ensures they remain available even if gh.env modifies PATH
if ! grep -Fqx "$path_line" "$bashrc_path"; then
  printf '%s\n' "$path_line" >> "$bashrc_path"
fi

# Update the git submodules
git submodule update --init --recursive