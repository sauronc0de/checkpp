#!/bin/bash

# Print enter on entrypoint bash script
echo "✅ Entering the docker entrypoint script"

# Check user
echo "👤 Current user: $(whoami)"

# Apply the correct permissions to the workspace directory
echo "🔐 Applying permissions to the: $WORKSPACE_DIR" 

# Set all files with the docker user as the owner
sudo chown --no-dereference -R $(whoami):$(whoami) $WORKSPACE_DIR

# Set executable permissions to all files within a relative path
sudo chmod -R +x ./tools/tasks
