#!/bin/sh
# Cross-compiles the Miyoo binary (build/miyoo/pocket-hustle) inside the toolchain container.
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
IMAGE=gta1-miyoo-toolchain

if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
	echo "build-miyoo: Docker image $IMAGE is missing (built by ../gta1-miyoo or ../scorched-emiyoolator)" >&2
	exit 1
fi

docker run --rm --user "$(id -u):$(id -g)" -v "$ROOT:/src" -w /src "$IMAGE" \
	make miyoo

file "$ROOT/build/miyoo/pocket-hustle" 2>/dev/null || true
