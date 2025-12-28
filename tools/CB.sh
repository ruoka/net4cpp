#!/usr/bin/env bash
# deps/net/tools/CB.sh — C++ Builder bootstrap for net submodule
# - Prefers a sibling tester (e.g. when net is used inside a larger repo with deps/tester)
# - Falls back to local deps/tester (for standalone net repo)
# - Optionally clones tester from GitHub if missing (export CB_FETCH_DEPS=1)

set -e

TOOLS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$TOOLS_DIR/.." && pwd)"
BIN="$TOOLS_DIR/cb"

# Default behavior:
# - On normal dev machines: run all tests (including network) by default.
# - In Cursor sandbox/CI-like environments: disable network tests unless user explicitly overrides.
if [[ -n "${CURSOR_SANDBOX:-}" && -z "${NET_DISABLE_NETWORK_TESTS+x}" ]]; then
    export NET_DISABLE_NETWORK_TESTS=1
fi

# Resolve tester root:
# 1) sibling deps/tester (common when PROJECT_ROOT is .../deps/net)
# 2) local deps/tester (standalone net repo)
TESTER_ROOT=""
if [[ -d "$PROJECT_ROOT/../tester/tools" ]]; then
    TESTER_ROOT="$(cd "$PROJECT_ROOT/../tester" && pwd)"
elif [[ -d "$PROJECT_ROOT/deps/tester/tools" ]]; then
    TESTER_ROOT="$(cd "$PROJECT_ROOT/deps/tester" && pwd)"
fi

if [[ -z "$TESTER_ROOT" ]]; then
    if [[ "${CB_FETCH_DEPS:-0}" == "1" ]]; then
        mkdir -p "$PROJECT_ROOT/deps"
        echo "tester dependency missing; cloning into '$PROJECT_ROOT/deps/tester'..."
        git clone --depth 1 "https://github.com/ruoka/tester.git" "$PROJECT_ROOT/deps/tester"
        TESTER_ROOT="$(cd "$PROJECT_ROOT/deps/tester" && pwd)"
    else
        echo "ERROR: tester dependency not found."
        echo "Looked for:"
        echo "  - $PROJECT_ROOT/../tester"
        echo "  - $PROJECT_ROOT/deps/tester"
        echo
        echo "Fix by initializing submodules or cloning tester:"
        echo "  git submodule update --init --recursive"
        echo "or rerun with:"
        echo "  CB_FETCH_DEPS=1 $0 $*"
        exit 1
    fi
fi

SRC="$TESTER_ROOT/tools/cb.c++"

# Detect OS and set compiler/LLVM paths
UNAME_OUT="$(uname -s)"
case "$UNAME_OUT" in
    Linux)
    CXX_COMPILER="clang++-20"
    LLVM_PREFIX="/usr/lib/llvm-20"
        STD_CPPM_DEFAULT="/usr/lib/llvm-20/share/libc++/v1/std.cppm"
        ;;
    Darwin)
    CXX_COMPILER="/usr/local/llvm/bin/clang++"
    LLVM_PREFIX="/usr/local/llvm"
        STD_CPPM_DEFAULT="/usr/local/llvm/share/libc++/v1/std.cppm"
        ;;
    *)
    echo "ERROR: Unsupported OS '$UNAME_OUT'"
    exit 1
        ;;
esac

# Ensure we always search the LLVM lib dir when linking CB itself
export LDFLAGS="-Wl,-rpath,$LLVM_PREFIX/lib ${LDFLAGS}"

# Rebuild if needed
if [[ ! -x "$BIN" ]] || [[ "$SRC" -nt "$BIN" ]]; then
    "$CXX_COMPILER" \
        -B"$LLVM_PREFIX/bin" \
        -std=c++23 -O3 -pthread \
        -fuse-ld=lld \
        -stdlib=libc++ \
        -I"$LLVM_PREFIX/include/c++/v1" \
        -L"$LLVM_PREFIX/lib" \
        -Wl,-rpath,"$LLVM_PREFIX/lib" \
        "$SRC" -o "$BIN"
fi

# Resolve std.cppm path
STD_CPPM=""
if [[ -n "$1" && ("$1" == *.cppm || "$1" == */*) ]]; then
        STD_CPPM="$1"
        shift
fi
if [[ -z "$STD_CPPM" ]]; then
    STD_CPPM="${LLVM_PATH:-$STD_CPPM_DEFAULT}"
fi

# Find project root and build include flags
CURRENT_DIR="$(pwd)"
INCLUDE_FLAGS=(
    -I "$PROJECT_ROOT/net"
)

# tester module include dir
if [[ -d "$TESTER_ROOT/tester" ]]; then
    INCLUDE_FLAGS+=(-I "$TESTER_ROOT/tester")
fi

# Run it
exec "$BIN" "$STD_CPPM" "${INCLUDE_FLAGS[@]}" --link-flags "-L/opt/homebrew/lib" "$@"
