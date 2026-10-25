#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

OUTPUT=$(printf 'help\ninfo\nexit\n' | ./ossp_monitor 2>&1 || true)

printf '%s\n' "$OUTPUT" | grep -q "LINUX SYSTEM OPERATIONS CONSOLE" || {
  echo "FAIL: missing console banner" >&2
  echo "$OUTPUT" >&2
  exit 1
}

printf '%s\n' "$OUTPUT" | grep -q "linux-os>" || {
  echo "FAIL: missing shell prompt" >&2
  echo "$OUTPUT" >&2
  exit 1
}

printf '%s\n' "$OUTPUT" | grep -q "Kernel Name" || {
  echo "FAIL: missing info output" >&2
  echo "$OUTPUT" >&2
  exit 1
}

echo "PASS"
