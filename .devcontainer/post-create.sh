#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

STAMP=".devcontainer/bootstrap-stamp"
KEY="$(git rev-parse HEAD)-$(git submodule status)"

echo "=== Toolchain ==="
clang++-21 -v
/usr/lib/llvm-21/bin/clang-scan-deps --version

if [[ -f /usr/lib/llvm-21/share/libc++/v1/std.cppm ]]; then
    echo "std.cppm: /usr/lib/llvm-21/share/libc++/v1/std.cppm"
else
    echo "WARNING: std.cppm not found at /usr/lib/llvm-21/share/libc++/v1/std.cppm"
    find /usr/lib/llvm-21 -name "std.cppm" 2>/dev/null || true
fi

echo "=== Submodules ==="
git submodule update --init --depth 1 deps/tester

if [[ -f "$STAMP" && "$(cat "$STAMP")" == "$KEY" ]]; then
    echo "=== Build ==="
    echo "Bootstrap up to date — skipping build (commit and submodules unchanged)"
else
    echo "=== Build ==="
    ./tools/CB.sh debug build
    echo "$KEY" > "$STAMP"
fi

echo "Dev container ready."