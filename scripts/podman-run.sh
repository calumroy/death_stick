#!/usr/bin/env bash
set -euo pipefail

IMAGE_NAME=${IMAGE_NAME:-death-stick-esp-idf}
IMAGE_TAG=${IMAGE_TAG:-latest}

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
REPO_ROOT=$(cd -- "$SCRIPT_DIR/.." &>/dev/null && pwd)

# Default to mapping the current user into the container for file permissions
USERNS_FLAG=${USERNS_FLAG:---userns=keep-id}

# Default devices and privileges for flashing
TTY_DEVICE=${TTY_DEVICE:-/dev/ttyACM0}
TTY_MAP="--device=$TTY_DEVICE:$TTY_DEVICE"

# Optional: mount podman unix socket for rootless USB access on some distros (not always needed)

mkdir -p "$REPO_ROOT/.espressif"

exec podman run --rm -it \
  $USERNS_FLAG \
  --name death-stick-dev \
  --hostname death-stick \
  --env IDF_PATH=/workspace/software/esp_idf \
  --env WORKSPACE=/workspace \
  -v "$REPO_ROOT:/workspace:Z" \
  -v "$REPO_ROOT/.espressif:/workspace/.espressif:Z" \
  $TTY_MAP \
  --group-add keep-groups \
  --security-opt label=disable \
  --cap-add SYS_RAWIO \
  "$IMAGE_NAME:$IMAGE_TAG" \
  bash


