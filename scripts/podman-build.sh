#!/usr/bin/env bash
set -euo pipefail

IMAGE_NAME=${IMAGE_NAME:-death-stick-esp-idf}
IMAGE_TAG=${IMAGE_TAG:-latest}

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
REPO_ROOT=$(cd -- "$SCRIPT_DIR/.." &>/dev/null && pwd)

cd "$REPO_ROOT"

echo "Building image $IMAGE_NAME:$IMAGE_TAG..."
podman build \
  -f "$REPO_ROOT/Containerfile" \
  -t "$IMAGE_NAME:$IMAGE_TAG" \
  --build-arg BASE_IMAGE=debian:bookworm \
  "$REPO_ROOT"

echo "Done."


