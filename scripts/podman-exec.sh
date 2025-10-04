#!/usr/bin/env bash
set -euo pipefail

CONTAINER_NAME=${CONTAINER_NAME:-death-stick-dev}

if ! podman ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
  echo "Container ${CONTAINER_NAME} not running. Start it with scripts/podman-run.sh" >&2
  exit 1
fi

exec podman exec -it "$CONTAINER_NAME" "$@"


