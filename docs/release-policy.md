# Release Policy

## Current status: v3.4.1

**[`v3.4.1`](https://github.com/ruoka/net4cpp/releases/tag/v3.4.1)** (24 October 2026) is the
current SemVer tag for the Clang 21 + libc++ modules consume surface (`import net;`).
It changes no library code. It fixes README examples that did not compile or did not
match (`cors_middleware`, `ws.send`, syslog setters, a route that missed query strings),
documents middlewares, the HTTP client and the thread-per-connection model, and compiles
every README section in the tests.
[`v3.4.0`](https://github.com/ruoka/net4cpp/releases/tag/v3.4.0) (23 October 2026) added `http::request` for handlers, WebSocket and SSE routes and gates, matched routes
in registration order with each regex compiled once, kept `controller&` references
stable, and added redirect and gateway status constants. It deprecated the duplicate
handler names; see [HTTP server API improvements](http-server-api-improvements.md).
[`v3.3.0`](https://github.com/ruoka/net4cpp/releases/tag/v3.3.0) (23 October 2026) bounded
how long one client can hold a connection, rejected smuggled and injected protocol
fields, and added `http::middleware::origin_gate` for browser-facing WebSocket and SSE routes.
[`v3.2.0`](https://github.com/ruoka/net4cpp/releases/tag/v3.2.0) (23 October 2026) added
`server::server_token()` so a product can replace or omit the `Server` header.
[`v3.1.0`](https://github.com/ruoka/net4cpp/releases/tag/v3.1.0) (23 October 2026) added the
HTTP client helper and the HTTP server defaults: a 1 MiB body cap, a 30s idle read timeout,
a 128-connection cap, and browser policy headers.
[`v3.0.1`](https://github.com/ruoka/net4cpp/releases/tag/v3.0.1) (2 August 2026) was the
previous patch. [`v3.0.0`](https://github.com/ruoka/net4cpp/releases/tag/v3.0.0) was the first
modules-era cut. Prior tags `v1.0` / `v2.0` / `v2.1` are historical.

Pin `v3.4.1` (or a later tag / deliberate commit). Nested `deps/tester` must match the
parent tree’s tester pin — see
[YarDB versioning](https://github.com/ruoka/YarDB/blob/master/docs/versioning.md).

## What is public

| Surface | Contract |
|---------|----------|
| `import net;` | Partitions re-exported by the net module (sockets, HTTP server/client pieces, middleware, SSE, MCP transport, structured log stream, etc.) as used by CI and the README examples |
| Build / test | `./tools/CB.sh` with project tag `[net]`; `NET_DISABLE_NETWORK_TESTS` as documented |

Private implementation details, spike/experimental notes in the README, and nested tester
docs are out of scope for net’s SemVer contract.

Some exported types are visible only so code can name them. Their documented use is
the contract, not their representation:

- `http::response_with_headers` (and its alias `http::response`): create with
  `http::make_response` / `http::make_error_response`; read with a structured binding
  `auto [status, content, headers] = ...`. That it is a `std::tuple`, brace
  construction, `std::get` and `std::tie` are not covered and may change in a minor
  release.

## Versioning

Semantic versioning (`vMAJOR.MINOR.PATCH`) on the public surface above.

| Bump | Means |
|------|-------|
| MAJOR | Incompatible change to the exported module API, or raising the minimum toolchain |
| MINOR | Additive, backward-compatible public change |
| PATCH | Fix that leaves documented behaviour intact |

**Deprecation.** A public name is marked `[[deprecated("use …")]]` in a minor release,
stays for at least one further minor release, and is removed only in a major release.

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
