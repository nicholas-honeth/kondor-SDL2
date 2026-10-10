#!/bin/bash
# Random-input sanitizer runs: fuzz.sh <kondor binary> <game dir> <first seed> <last seed> <frames> [jobs]
#
# Each seed gets a fresh copy of the game directory (the game rewrites KONDOR.CFG/.HSC), a key
# script from fuzzkeys.py, and one headless run without frame pacing. Runs that end before
# <frames> picked EXIT in the menu, which is a normal outcome. Set KONDOR_NOSOUND=0 in the
# environment to run the sound path too (SDL's dummy audio driver is used).
set -u
bin=$(readlink -f "$1"); game=$(readlink -f "$2"); first=$3; last=$4; frames=$5; jobs=${6:-4}
tools=$(dirname "$(readlink -f "$0")")
out=${FUZZ_OUT:-$PWD/fuzz-out}
mkdir -p "$out"
nosound=${KONDOR_NOSOUND:-1}

run_one() {
    local s=$1 dir="$out/game$1"
    rm -rf "$dir" && cp -r "$game" "$dir"
    python3 "$tools/fuzzkeys.py" "$s" "$frames" > "$out/script$s.txt"
    local env=(KONDOR_HEADLESS=1 KONDOR_FAST=1 KONDOR_SEED=$s KONDOR_SCRIPT=$out/script$s.txt
               KONDOR_MAXFRAMES=$frames SDL_AUDIODRIVER=dummy
               ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1)
    [ "$nosound" = 0 ] || env+=(KONDOR_NOSOUND=1)
    (cd "$dir" && env "${env[@]}" timeout -s KILL 900 "$bin" > "$out/log$s.txt" 2>&1)
    local rc=$?
    echo "seed $s rc=$rc sanitizer_reports=$(grep -c 'runtime error\|AddressSanitizer' "$out/log$s.txt")" \
         "$(grep -o 'exit after [0-9]* frames' "$out/log$s.txt")"
    rm -rf "$dir"
}

n=0
for s in $(seq "$first" "$last"); do
    run_one "$s" &
    n=$((n + 1))
    if (( n % jobs == 0 )); then wait; fi
done
wait
