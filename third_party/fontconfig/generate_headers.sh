#!/bin/bash
#
# Regenerate fontconfig's checked-in derived headers from the src/ submodule.
# fontconfig normally produces these during its meson build; we don't run meson,
# so this script reproduces just the mechanical parts. config.h is NOT generated
# here -- it's hand-maintained (see README.md).
#
# Run after bumping the src/ submodule, then rebuild all targets to catch any
# config gaps the new version introduces.
#
# Requires: python3, gperf, and a C compiler (cc).
set -euo pipefail

cd "$(dirname "$0")"
root="$(pwd)"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

for tool in python3 gperf cc; do
    command -v "$tool" >/dev/null || { echo "error: '$tool' not found" >&2; exit 1; }
done

# fcstdint.h -- just an <stdint.h> shim upstream fills in per-platform.
cp src/src/fcstdint.h.in fcstdint.h
echo "  fcstdint.h"

# Public header: substitute the cache versions, read from meson.build so a
# version bump flows through automatically. nextcacheversion is 0 unless a cache
# snapshot is in progress (see the meson.build logic).
cacheversion="$(sed -n 's/^cacheversion = \([0-9]*\).*/\1/p' src/meson.build)"
snapversion="$(sed -n 's/^cachesnapversion = \([0-9]*\).*/\1/p' src/meson.build)"
if [ "$snapversion" -gt 0 ]; then
    nextversion="$((cacheversion + 1))"
else
    nextversion=0
fi
sed -e "s/@CACHE_VERSION@/$cacheversion/" \
    -e "s/@CACHE_SNAP_VERSION@/$snapversion/" \
    -e "s/@NEXT_CACHE_VERSION@/$nextversion/" \
    src/fontconfig/fontconfig.h.in > include/fontconfig/fontconfig.h
echo "  fontconfig/fontconfig.h (cache $cacheversion/$snapversion/$nextversion)"

# Language orthography tables. The orth-file order is load-bearing ("Do not
# reorder, magic"), so take the list straight from meson.build in order. Left
# unquoted for word-splitting into args (orth filenames have no spaces); kept
# bash-3.2 compatible (no mapfile) for the stock macOS shell.
orth="$(grep -oE "'[^']+\.orth'" src/fc-lang/meson.build | tr -d "'")"
( cd src/fc-lang && python3 fc-lang.py $orth \
    --template fclang.tmpl.h --directory . \
    --output "$root/include/src/fclang.h" )
echo "  include/src/fclang.h ($(printf '%s\n' "$orth" | wc -l | tr -d ' ') orthographies)"

# Case-folding table.
( cd src/fc-case && python3 fc-case.py CaseFolding.txt \
    --template fccase.tmpl.h --output "$root/include/fc-case/fccase.h" )
echo "  include/fc-case/fccase.h"

# Object-name perfect hash: preprocess the gperf template (expanding fcobjs.h
# and fontconfig.h), strip the CUT_OUT block, then run gperf.
cc -E -P -I include -I src src/src/fcobjshash.gperf.h -o "$tmp/fcobjshash.i"
python3 src/src/cutout.py "$tmp/fcobjshash.i" "$tmp/fcobjshash.gperf"
gperf --pic -m 100 "$tmp/fcobjshash.gperf" --output-file include/src/fcobjshash.h
echo "  include/src/fcobjshash.h"

# Constant-name symbols.
python3 src/fc-const/fc-const.py src/fc-const/fcconst.list src/src/fcobjs.h \
    --output include/src/fcconst.h
echo "  include/src/fcconst.h"

# Generic-family perfect hash.
python3 src/fc-genericfamily/fc-genericfamily.py -d src/fc-genericfamily \
    -o "$tmp/fcgenericfamily.gperf"
gperf --pic -m 100 "$tmp/fcgenericfamily.gperf" \
    --output-file include/src/fcgenericfamily.h
echo "  include/src/fcgenericfamily.h"

# Symbol-alias headers: intentionally empty for a static library (see README).
for f in fcalias fcaliastail fcftalias fcftaliastail; do
    echo '/* Intentionally empty: symbol-hiding aliases are unnecessary for a static library. See ../../README.md. */' \
        > "include/src/$f.h"
done
echo "  include/src/fc{,ft}alias{,tail}.h (empty)"

# Guard the one cross-file invariant a bump can silently break: config.h's
# FC_GPERF_SIZE_T must match the length type gperf emitted (unsigned int for
# gperf 3.0.x, size_t for >= 3.1).
gperf_type="$(grep -hoE 'register [a-z ]+ len\)' include/src/fcobjshash.h | head -1 | sed -E 's/register (.+) len\)/\1/')"
config_type="$(sed -n 's/^#define FC_GPERF_SIZE_T //p' config.h | sed 's/ *$//')"
if [ "$gperf_type" != "$config_type" ]; then
    echo >&2
    echo "warning: gperf emitted '$gperf_type len' but config.h has" >&2
    echo "         FC_GPERF_SIZE_T = '$config_type'. Update config.h to match." >&2
fi

echo "Done."
