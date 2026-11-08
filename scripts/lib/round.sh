#!/bin/sh
# Reads a round file (tests/golden/rounds/NAME.keys) for the play-test scripts. Source it, then
#   round_load FILE
# sets ROUND_EXPECT (the "@expect" line: the final state the game's --summary must report),
# ROUND_SEED, ROUND_PARAMS ("--param A=1 --param B=2" or ""), ROUND_KEYS (the tokens,
# space separated), ROUND_NKEYS, ROUND_FRAMES (frames that play every key at the default
# pace and draw the last result) and ROUND_SHOTS ("label:N label:N": the screen after the
# first N keys).
#
# The file: "#" starts a comment line; "@seed N", "@param NAME=VALUE", "@expect day=D city=C
# cash=N bank=N debt=N over=0|1" and "@shot LABEL" are directives (the shot is of the screen after the keys above it); every other line is
# button tokens (U D L R A B X Y L1 R1 SEL START).

round_load() {
	[ -r "$1" ] || { echo "round: cannot read $1" >&2; return 1; }
	ROUND_SEED=$(awk '$1 == "@seed" { print $2 }' "$1")
	[ -n "$ROUND_SEED" ] || { echo "round: $1 has no @seed" >&2; return 1; }
	ROUND_EXPECT=$(awk '$1 == "@expect" { sub(/^@expect[ \t]+/, ""); print }' "$1")
	[ -n "$ROUND_EXPECT" ] || { echo "round: $1 has no @expect" >&2; return 1; }
	ROUND_PARAMS=$(awk '$1 == "@param" { printf "--param %s ", $2 }' "$1" | sed 's/ $//')
	ROUND_KEYS=$(awk '/^#/ || /^@/ || NF == 0 { next } { for (i = 1; i <= NF; i++) printf "%s ", $i }' "$1" | sed 's/ $//')
	ROUND_NKEYS=$(echo "$ROUND_KEYS" | wc -w)
	ROUND_FRAMES=$((ROUND_NKEYS * 3 + 6))
	ROUND_SHOTS=$(awk '$1 == "@shot" { printf "%s:%d ", $2, n } !/^#/ && !/^@/ && NF { n += NF }' "$1" | sed 's/ $//')
}

# round_file NAME_OR_PATH: the round file of a name (tests/golden/rounds/NAME.keys) or a path.
round_file() {
	case "$1" in
	*/* | *.keys) echo "$1" ;;
	*) echo "$ROOT/tests/golden/rounds/$1.keys" ;;
	esac
}

# round_name FILE: the round's name, e.g. okc-dallas-seattle.
round_name() {
	basename "$1" .keys
}

# round_all: the names of every round, one per line.
round_all() {
	for f in "$ROOT"/tests/golden/rounds/*.keys; do
		round_name "$f"
	done
}

# round_check NAME SUMMARY_FILE WHERE: 0 when the game's --summary line equals the round's @expect.
round_check() {
	got="$(cat "$2" 2>/dev/null || true)"
	if [ "$got" = "$ROUND_EXPECT" ]; then
		return 0
	fi
	echo "round: $1 on $3 ended in a different state" >&2
	echo "  expected: $ROUND_EXPECT" >&2
	echo "  got:      ${got:-(no summary written)}" >&2
	return 1
}
