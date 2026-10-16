#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
BIN="$ROOT/build"

fail() {
    printf 'FAIL: %s\n' "$1" >&2
    exit 1
}

contains() {
    case "$1" in
        *"$2"*) ;;
        *) fail "$3 (missing: $2)" ;;
    esac
}

out=$(printf '%s\n' 'input-one' | "$BIN/Skill-1")
contains "$out" 'Input: input-one' 'Skill-1 input'

out=$(printf '%s\n' 'exit' | timeout 3 "$BIN/Skill-2")
contains "$out" 'SHELLFORGE' 'Skill-2 exit'
out=$(timeout 3 "$BIN/Skill-2" </dev/null)

out=$(printf '%s\n' 'echo alpha' 'history' 'exit' | timeout 3 "$BIN/Skill-3")
contains "$out" '1: echo alpha' 'Skill-3 history'
out=$(printf '%s\n' 'echo one two' | "$BIN/Skill-3b")
contains "$out" 'Token: two' 'Skill-3b tokenizer'
out=$(printf 'one\ttwo\n' | "$BIN/Skill-4")
contains "$out" 'argc = 2' 'Skill-4 whitespace'
out=$(printf '%s\n' 'echo valid' | "$BIN/Skill-5")
contains "$out" 'Valid command' 'Skill-5 valid command'
if printf '\n' | "$BIN/Skill-5" >/dev/null 2>&1; then
    fail 'Skill-5 must reject an empty command'
fi
out=$(printf "echo \"two words\" 'literal \$HOME' escaped\\ value\n" | "$BIN/Skill-6")
contains "$out" 'argv[1] = <two words>' 'Skill-6 quoted argument'
contains "$out" 'argv[2] = <literal $HOME>' 'Skill-6 single quotes'
contains "$out" 'argv[3] = <escaped value>' 'Skill-6 escaped whitespace'
out=$(timeout 3 "$BIN/Skill-7")
contains "$out" 'Child process:' 'Skill-7 fork child'
out=$(timeout 3 "$BIN/Skill-8")
contains "$out" 'Child replaced by echo' 'Skill-8 exec'
contains "$out" 'Child exit status: 0' 'Skill-8 wait status'
out=$(printf '%s\n' 'printf path-search' 'exit' | timeout 3 "$BIN/Skill-9")
contains "$out" 'path-search' 'Skill-9 PATH execution'
out=$(printf '%s\n' '$HOME' | "$BIN/Skill-10")
contains "$out" "$HOME" 'Skill-10 environment expansion'
out=$(printf '%s\n' 'pwd' 'cd /tmp' 'pwd' 'exit' | timeout 3 "$BIN/Skill-11")
contains "$out" '/tmp' 'Skill-11 directory built-ins'
out=$(printf '%s\n' 'export OSSP_TEST=value' 'exit' | "$BIN/Skill-12")
contains "$out" 'OSSP_TEST=value' 'Skill-12 export'
out=$(printf '%s\n' 'first-command' '!!' 'history' 'exit' | "$BIN/Skill-13")
contains "$out" 'Last command: first-command' 'Skill-13 recall'
contains "$out" '1  first-command' 'Skill-13 history list'
out=$(printf '%s\n' 'printf abc | wc -c' | timeout 3 "$BIN/Skill-14")
contains "$out" '3' 'Skill-14 single pipe'
out=$(printf '%s\n' 'printf abc | tr a-z A-Z | wc -c' | timeout 3 "$BIN/Skill-15")
contains "$out" '3' 'Skill-15 pipeline'
printf 'echo redirected > %s\n' "$TMP/out" | timeout 3 "$BIN/Skill-16" >/dev/null
[ "$(cat "$TMP/out")" = redirected ] || fail 'Skill-16 output redirection'
printf 'echo first >> %s\n' "$TMP/append" | timeout 3 "$BIN/Skill-17" >/dev/null
printf 'ls /path-that-does-not-exist 2> %s\n' "$TMP/error" | timeout 3 "$BIN/Skill-17" >/dev/null 2>&1 || :
[ "$(cat "$TMP/append")" = first ] || fail 'Skill-17 append redirection'
[ -s "$TMP/error" ] || fail 'Skill-17 error redirection'
out=$(printf '%s\n' 'true &' | timeout 3 "$BIN/Skill-18")
contains "$out" 'started background pid' 'Skill-18 background launch'
out=$(printf '%s\n' 'sleep 0.2 &' 'jobs' 'exit' | timeout 3 "$BIN/Skill-19")
contains "$out" 'started sleep' 'Skill-19 job tracking'
out=$(printf '%s\n' 'true' 'exit' | timeout 3 "$BIN/Skill-20")
contains "$out" 'Skill 20' 'Skill-20 signal shell'
out=$(printf '%s\n' 'true' | timeout 3 "$BIN/Skill-21")
contains "$out" 'foreground process group' 'Skill-21 process group'
out=$(printf '%s\n' 'sleep 1 &' 'exit' | timeout 3 "$BIN/Skill-22")
contains "$out" 'started' 'Skill-22 background job'
out=$(printf '%s\n' 'printf abc | tr a-z A-Z' | timeout 3 "$BIN/Skill-23")
contains "$out" 'ABC' 'Skill-23 integrated pipeline'
out=$(printf '%s\n' 'export OSSP_TEST=2520030030' 'echo "$OSSP_TEST"' 'printf abc | tr a-z A-Z' 'command_that_does_not_exist' 'echo $?' 'exit' | timeout 5 "$BIN/ShellForge" 2>&1)
contains "$out" '2520030030' 'ShellForge environment and identity'
contains "$out" 'ABC' 'ShellForge pipeline'
contains "$out" '127' 'ShellForge exit status'
out=$(printf 'echo final > %s\ncat < %s\nexit\n' "$TMP/sf" "$TMP/sf" | timeout 5 "$BIN/ShellForge")
contains "$out" 'final' 'ShellForge redirection'
out=$(printf '%s\n' 'sleep 0.1 &' 'jobs' 'exit' | timeout 5 "$BIN/ShellForge")
contains "$out" '[1]' 'ShellForge background job'
if command -v script >/dev/null 2>&1; then
    out=$(
        {
            printf 'sleep 10\n'
            sleep 0.3
            printf '\032'
            sleep 0.3
            printf 'bg\n'
            sleep 0.3
            printf 'fg\n'
            sleep 0.3
            printf '\003'
            sleep 0.3
            printf 'exit\n'
        } | timeout 8 script -q -c "$BIN/ShellForge" /dev/null
    )
    contains "$out" 'stopped' 'ShellForge Ctrl-Z job stop'
    contains "$out" 'continued' 'ShellForge bg resume'
fi

printf 'PASS: all 24 numbered stages and Skill-3b smoke tests\n'
