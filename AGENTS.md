# AGENTS.md

## Cursor Cloud specific instructions

`net4cpp` is a single **C++23 modules** networking + HTTP library (no databases, no
long-running services). It builds to prebuilt module files (`.pcm`), object files, and a
test runner binary. There is no npm/pip/cargo — the only external dependency is the
`deps/tester` git submodule (test framework + build core), fetched by the update script.

### Toolchain
- Requires **Clang/LLVM 21** with **libc++** (`clang++-21`, `libc++-21-dev`,
  `libc++abi-21-dev`, `lld-21`, plus `clang-tidy-21`/`cppcheck` for lint). These are
  installed in the VM image; the update script only refreshes the `deps/tester` submodule.
- `tools/CB.sh` hardcodes `clang++-21` and `/usr/lib/llvm-21` (it ignores `CXX`), so you do
  not need to export `CC`/`CXX`/`LDFLAGS`.

### Build / test / lint (see `README.md` for the full story)
- Build: `./tools/CB.sh debug build` (or `release`).
- Test: `./tools/CB.sh debug test`.
- Lint (non-blocking, mirrors `.github/workflows/ci.yml`): `cppcheck` over `net/ tools/`,
  and `clang-tidy-21 <file> -- -std=c++23 -I/usr/lib/llvm-21/include/c++/v1`. Note
  `clang-tidy` reports `module 'std' not found` on the `import std;` line — this is
  expected (clang-tidy doesn't consume the prebuilt `std.pcm`) and CI treats it as
  non-blocking.

### Network tests — important gotcha
- `tools/CB.sh` only auto-sets `NET_DISABLE_NETWORK_TESTS=1` when the `CURSOR_SANDBOX`
  env var is present. In this cloud environment `CURSOR_SANDBOX` is **not** set, so network
  tests run by default and a couple fail against the restricted sandbox network (e.g.
  `net-posix.test.c++` connect-timeout to a non-routable IP, `net-socket.test.c++`
  `wait_for`). These are environment limitations, not code bugs.
- For a clean, fully-green run use: `NET_DISABLE_NETWORK_TESTS=1 ./tools/CB.sh debug test`
  (393/393 tests pass). **Loopback** (`::1` / `127.0.0.1`) networking does work, so
  echo/HTTP-over-loopback demos run fine.
