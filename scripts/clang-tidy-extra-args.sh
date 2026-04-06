#!/bin/sh

set -eu

compiler="${1:-clang}"

"$compiler" -E -x c - -v < /dev/null 2>&1 | awk '
  /#include <...> search starts here:/ { capture = 1; next }
  /End of search list./ { capture = 0 }
  capture && $0 ~ /^ / {
    gsub(/^ +/, "", $0)
    gsub(/ +$/, "", $0)
    printf "--extra-arg=-isystem%s ", $0
  }
'
