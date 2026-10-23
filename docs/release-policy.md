# Release Policy

## Current status: v3.2.0

**[`v3.2.0`](https://github.com/ruoka/net4cpp/releases/tag/v3.2.0)** (23 October 2026) is the
current SemVer tag for the Clang 21 + libc++ modules consume surface (`import net;`).
It adds `server::server_token()` so a product can replace or omit the `Server` header.
The default token remains `net`.
[`v3.1.0`](https://github.com/ruoka/net4cpp/releases/tag/v3.1.0) (23 October 2026) added the
HTTP client helper and the HTTP server defaults: a 1 MiB body cap, a 30s idle read timeout,
a 128-connection cap, and browser policy headers.
[`v3.0.1`](https://github.com/ruoka/net4cpp/releases/tag/v3.0.1) (2 August 2026) was the
previous patch. [`v3.0.0`](https://github.com/ruoka/net4cpp/releases/tag/v3.0.0) was the first
modules-era cut. Prior tags `v1.0` / `v2.0` / `v2.1` are historical.

Pin `v3.2.0` (or a later tag / deliberate commit). Nested `deps/tester` must match the
parent tree’s tester pin — see
[YarDB versioning](https://github.com/ruoka/YarDB/blob/master/docs/versioning.md).

## What is public

| Surface | Contract |
|---------|----------|
| `import net;` | Partitions re-exported by the net module (sockets, HTTP server/client pieces, middleware, SSE, MCP transport, structured log stream, etc.) as used by CI and the README examples |
| Build / test | `./tools/CB.sh` with project tag `[net]`; `NET_DISABLE_NETWORK_TESTS` as documented |

Private implementation details, spike/experimental notes in the README, and nested tester
docs are out of scope for net’s SemVer contract.

## Versioning

Semantic versioning (`vMAJOR.MINOR.PATCH`) on the public surface above.

| Bump | Means |
|------|-------|
| MAJOR | Incompatible change to the exported module API, or raising the minimum toolchain |
| MINOR | Additive, backward-compatible public change |
| PATCH | Fix that leaves documented behaviour intact |

**Modules-era major.** `v3.0.0` is the modules consume-surface line relative to `v1`/`v2`.

**Tester pin.** When bumping `deps/tester`, land the same SHA in cryptic, net4cpp,
json4cpp, and YarDB together.

**No bump required.** Pin-only `deps/tester` syncs and test-only / docs-only /
CI-only changes that leave the public surface above unchanged do **not** require a
new SemVer tag. See
[YarDB versioning — When not to bump](https://github.com/ruoka/YarDB/blob/master/docs/versioning.md#when-not-to-bump).

## Release criteria

1. Default-branch CI green (`debug`/`release`, `--jsonl=failures`, `--jobs=$(nproc)`,
   `--tags='\[net\]'`).
2. Open PRs that affect the release surface are merged or closed.
3. Nested `deps/tester` matches the ecosystem pin rule.
4. README / this policy agree with the tagged surface; minimum toolchain stated.

## Cutting a release

```bash
NET_DISABLE_NETWORK_TESTS=1 ./tools/CB.sh debug test --jsonl=failures --jobs="$(nproc)" --tags='\[net\]'
git tag -a vX.Y.Z -m "vX.Y.Z"
git push origin vX.Y.Z
gh release create vX.Y.Z --title vX.Y.Z --notes-file release-notes.md
```

Release notes: breaking changes, additions, fixes, minimum compiler, tester pin SHA/tag.
