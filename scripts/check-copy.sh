#!/bin/sh
# Fails when any tracked file, or the commit message file given as $1, contains an
# em dash (U+2014) or an en dash (U+2013). The project's copy uses neither.
set -u
cd "$(dirname "$0")/.."

list=$(git ls-files)
if [ $# -ge 1 ]; then
	list="$list
$1"
fi

hits=$(printf '%s\n' "$list" | grep -v '^$' | while IFS= read -r f; do
	[ -f "$f" ] || continue
	LC_ALL=C grep -nHP '\xE2\x80[\x93\x94]' -- "$f"
done)

if [ -n "$hits" ]; then
	printf '%s\n' "$hits"
	echo "check-copy: em or en dash found (rewrite the sentence instead)" >&2
	exit 1
fi
echo "check-copy: ok"
