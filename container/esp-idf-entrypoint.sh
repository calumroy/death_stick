#!/usr/bin/env bash
set -euo pipefail

# Ensure workspace is mounted at /workspace
export WORKSPACE=${WORKSPACE:-/workspace}
export IDF_PATH=${IDF_PATH:-/workspace/software/esp_idf}

if [[ ! -d "$WORKSPACE" ]]; then
  echo "Workspace directory $WORKSPACE is missing. Mount your repo to /workspace." >&2
  exit 1
fi

# If ESP-IDF exists in the mounted workspace, prepare its tools
if [[ -d "$IDF_PATH" ]]; then
  # Some ESP-IDF helper scripts expect bash
  if [[ -f "$IDF_PATH/export.sh" ]]; then
    # Create a per-container tools directory and keep it cached under /home/$USER/.espressif
    export IDF_TOOLS_PATH=${IDF_TOOLS_PATH:-$WORKSPACE/.espressif}
    mkdir -p "$IDF_TOOLS_PATH"
    # Install tools if missing
    if [[ ! -d "$IDF_TOOLS_PATH/tools" ]]; then
      echo "Installing ESP-IDF tools into $IDF_TOOLS_PATH..."
      bash "$IDF_PATH/install.sh" >/tmp/esp-idf-install.log 2>&1 || {
        echo "ESP-IDF install failed. See /tmp/esp-idf-install.log" >&2
        exit 1
      }
    fi
    # Export environment for current shell
    # shellcheck disable=SC1090
    source "$IDF_PATH/export.sh"
  else
    echo "ESP-IDF export.sh not found at $IDF_PATH. Ensure submodule is checked out." >&2
  fi
else
  echo "ESP-IDF directory not found at $IDF_PATH."
fi

exec "$@"


