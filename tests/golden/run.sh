#!/bin/sh
# Golden transcript: a scripted 15-day game with a fixed seed must print exactly
# tests/golden/basic.out. After a deliberate rules or text change, regenerate it with
#   build/host/ph-text --seed 1 --days 15 --name Golden --script < tests/golden/basic.cmds > tests/golden/basic.out
# and read the diff before committing it.
set -eu
cd "$(dirname "$0")/../.."
make -s build/host/ph-text
mkdir -p build
build/host/ph-text --seed 1 --days 15 --name Golden --script < tests/golden/basic.cmds > build/golden-basic.out
diff -u tests/golden/basic.out build/golden-basic.out
echo "golden: ok"
