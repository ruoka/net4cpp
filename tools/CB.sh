#!/usr/bin/env bash
# deps/net/tools/CB.sh — net bootstrap (library + tester)

set -e

TOOLS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$TOOLS_DIR/.." && pwd)"

TESTER_ROOT=""
if [[ -d "$PROJECT_ROOT/../tester/tools" ]]; then
  TESTER_ROOT="$(cd "$PROJECT_ROOT/../tester" && pwd)"
elif [[ -d "$PROJECT_ROOT/deps/tester/tools" ]]; then
  TESTER_ROOT="$(cd "$PROJECT_ROOT/deps/tester" && pwd)"
fi

CB_TOOLS_DIR="$TOOLS_DIR"
CB_PROJECT_ROOT="$PROJECT_ROOT"
CB_TESTER_ROOT="$TESTER_ROOT"
CB_SANDBOX_DISABLE_NETWORK_TESTS=1
CB_RESPECT_CXX_ENV=0
CB_LINK_FLAGS="-L/opt/homebrew/lib"

# Prefer the pinned submodule. A sibling checkout is the YarDB layout
# (deps/net next to deps/cryptic) when this tree has not initialized its own.
CRYPTIC_ROOT=""
if [[ -d "$PROJECT_ROOT/deps/cryptic/cryptic" ]]; then
  CRYPTIC_ROOT="$(cd "$PROJECT_ROOT/deps/cryptic" && pwd)"
elif [[ -d "$PROJECT_ROOT/../cryptic/cryptic" ]]; then
  CRYPTIC_ROOT="$(cd "$PROJECT_ROOT/../cryptic" && pwd)"
fi

if [[ -z "$CRYPTIC_ROOT" ]]; then
  echo "ERROR: cryptic dependency not found."
  echo "Looked for:"
  echo "  - $PROJECT_ROOT/deps/cryptic"
  echo "  - $PROJECT_ROOT/../cryptic"
  echo
  echo "Fix by initializing submodules:"
  echo "  git submodule update --init --recursive"
  exit 1
fi

CB_INCLUDE_DIRS=("$PROJECT_ROOT/net" "$CRYPTIC_ROOT/cryptic")
[[ -n "$TESTER_ROOT" && -d "$TESTER_ROOT/tester" ]] && CB_INCLUDE_DIRS+=("$TESTER_ROOT/tester")

CB_CORE=""
if [[ -n "$TESTER_ROOT" ]]; then
  CB_CORE="$TESTER_ROOT/tools/CB.sh.core"
elif [[ -f "$PROJECT_ROOT/deps/tester/tools/CB.sh.core" ]]; then
  CB_CORE="$PROJECT_ROOT/deps/tester/tools/CB.sh.core"
else
  CB_CORE="$PROJECT_ROOT/../tester/tools/CB.sh.core"
fi
source "$CB_CORE"