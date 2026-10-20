#!/bin/sh
# Optional source directory supports a red/green run against another revision.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
source_dir=${ENRMRKD_SOURCE_DIR:-$root/src}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
"${CC:-cc}" -std=c99 -Wall -Wextra -Wshadow -Wdeclaration-after-statement \
    -I "$source_dir" "$root/test/html-br.c" "$source_dir/enrmrkd.c" -o "$work/html-br"
"$work/html-br"
