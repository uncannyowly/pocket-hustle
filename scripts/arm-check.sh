#!/bin/sh
# Compiles and links the test runner for the Miyoo Mini (32-bit ARM) inside the
# toolchain container the other Mini ports use. Proves the core stays portable.
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
IMAGE=gta1-miyoo-toolchain

if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
	echo "arm-check: Docker image $IMAGE is missing (built by ../gta1-miyoo or ../scorched-emiyoolator)" >&2
	exit 1
fi

docker run --rm --user "$(id -u):$(id -g)" -v "$ROOT:/src" -w /src "$IMAGE" \
	make arm-check-inner

file "$ROOT/build/arm/run-tests"
