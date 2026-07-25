# Implementation plan: SSE then MCP on `http::server`

**Branch:** `feature/sse-mcp-server`  
**Repo:** [ruoka/net4cpp](https://github.com/ruoka/net4cpp)  
**Status:** M1–M3 complete (SSE v1); M4 session transport on `feature/mcp-sse-transport`  
**Date:** 2026-07-25

## Goal

Evaluate and plan extending net4cpp’s `http::server` to support:

1. **SSE (Server-Sent Events)** — first-class, reusable streaming responses  
2. **MCP (Model Context Protocol)** — built on that SSE (and related HTTP) transport so C++ services (YarDB, others) can expose agent tools without a Python bridge

This document is the design record for the branch. Implementation should follow the phased milestones below.

---

## Verdict (short)

| Capability | Feasible in net4cpp? | Notes |
|------------|----------------------|--------|
| **SSE** | **Yes** | Natural fit; WebSocket already models “take over the connection after headers.” |
| **MCP (SSE transport)** | **Yes, after SSE** | Protocol layer on top of SSE + ordinary `POST`; not a second socket stack. |
| **MCP (stdio)** | Out of scope for `http::server` | Process stdin/stdout; keep as app/tooling (e.g. YarDB `tools/*_mcp.py`) or a separate small module later. |
| **MCP (Streamable HTTP)** | Later optional | Newer MCP transport; plan SSE first for Cursor/`mcp` SDK parity with current YarDB bridge. |

**Recommendation:** implement **SSE v1 in net**, then **MCP SSE transport v1** (session + JSON-RPC tools) as `net:mcp` (or `http::mcp`) consuming the SSE primitives. Do **not** try to express SSE as today’s `callback → (status, content, headers)` fully-buffered response.

---

## Current server constraints (investigation)

Relevant code: `net/net-http_server.c++m`, WebSocket spike in README.

### What works today

- Thread-per-connection accept loop; keep-alive request loop on one `endpointstream`.
- Router: method + regex path → `controller` with buffered HTTP callback.
- **WebSocket precedent:** on `Upgrade: websocket`, server writes `101`, then calls `net::websocket::run_text_session(stream, handler)` and **leaves the normal request/response path**. That is the right pattern for long-lived SSE.
- Middleware stack (`cors`, body size, rate limit, metrics, …) runs for normal HTTP; upgrades are special-cased before body handling / route render.

### What blocks SSE today

1. **Handlers return a complete body** (`response_with_headers` / `content` as `std::string`). The write path always emits `Content-Length: <size>` and then the whole body once (`~L1023–1035` in `net-http_server.c++m`).
2. **No streaming response API** — no way for a handler to write headers, then push multiple chunks over time, then close.
3. **Request `Transfer-Encoding` is rejected** (Content-Length-only bodies). That is fine for SSE *responses* if we omit `Content-Length` and stream until close (common SSE pattern). Chunked *response* TE is optional later, not required for v1.
4. **Middleware timing** assumes a finite request → finite response; long-lived streams need different metrics/logging (start vs end duration, bytes streamed).

None of these are architectural dead-ends; they require an explicit **stream takeover** path similar to WebSocket.

---

## Phase 1 — SSE (Server-Sent Events)

### Spec subset (v1)

Implement enough of [HTML Living Standard — Server-sent events](https://html.spec.whatwg.org/multipage/server-sent-events.html) for agent/tooling use:

| Item | v1 |
|------|----|
| `Content-Type: text/event-stream` | Required |
| `Cache-Control: no-cache` | Required |
| `Connection: keep-alive` | Required |
| Event fields `data`, `event`, `id`, `retry` | Support writer helpers |
| Multi-line `data:` | Support |
| Comment lines (`:`) | Optional (heartbeats) |
| `Last-Event-ID` request header | Read and pass to handler (resume is app policy) |
| Automatic reconnect | Client-side; server only exposes `id` / `retry` |

### API sketch (net)

Prefer a **connection takeover** registration, mirroring WebSocket:

```cpp
// Illustrative — names TBD during implementation
server.sse("/events").sse([](http::sse::session& session, http::request_view, http::headers&) {
    session.send_comment("ok");
    session.send_event(/*event*/ "tick", /*data*/ "1", /*id*/ "1");
    // return when done; or loop until session.closed()
});
```

Suggested types (new module e.g. `net:sse` / `net-sse.c++m`):

- `http::sse::writer` / `session` — owns or borrows `endpointstream` after response headers are written  
  - `send_event(event, data, id = {})`  
  - `send_comment(text)`  
  - `flush()`  
  - `closed()` / detect write failure  
- Framing helpers that write `event:` / `data:` / `id:` / blank line per SSE rules (UTF-8 text).
- Server integration: after matching `GET` + SSE route (Accept optional), write response head **without** `Content-Length`, then run session callback, then close connection (or return to keep-alive only if we document “one SSE stream ends the HTTP session” — v1: **close after SSE ends**, simplest and safest).

### Server integration points

1. Detect SSE route **before** calling buffered `render()` (same region as WebSocket upgrade, after headers parsed).
2. Write:

   ```http
   HTTP/1.1 200 OK
   Content-Type: text/event-stream
   Cache-Control: no-cache
   Connection: keep-alive
   ```

   plus Date/Server. **CORS:** reuse `cors_middleware` (same allowlist / ACAO behavior as normal routes). Ensure the SSE takeover path still runs the middleware chain for the opening `GET` (or applies equivalent CORS header injection from the shared helpers) so browser EventSource clients are not special-cased with a second policy.
3. Disable keep-alive request loop for that connection after the stream ends (`break` out of the read loop), matching WebSocket.
4. Logging: `HTTP_SSE_OPEN` / `HTTP_SSE_CLOSE` with `request_id`, duration, bytes_out.

### Explicit non-goals for SSE v1

- HTTP/2 server push / multiplexing  
- Compressing `text/event-stream` (problematic with intermediaries)  
- Multipart or binary SSE  
- Reverse-proxy-specific workarounds beyond documenting `X-Accel-Buffering: no` as an optional header apps may set

### Tests (SSE)

Co-located `net-sse.test.c++` / extend `net-http_server.test.c++`:

- Client `GET /sse` receives headers + one/two events; parse framing.  
- Client disconnect mid-stream: server exits session without hang (use acceptor wake / write-fail path).  
- Heartbeat comment lines flush and do not break framing.  
- `Last-Event-ID` visible to handler.  
- Regression: normal JSON routes still send `Content-Length` and keep-alive.

### Docs

- README section “Server-Sent Events (v1)” parallel to WebSocket.  
- Note interaction with CORS and reverse proxies.

**Exit criteria Phase 1:** green `[net]` tests; example echo/ticker SSE endpoint; YarDB *not* required to adopt yet.

---

## Phase 2 — MCP over HTTP/SSE

### Context (ecosystem)

YarDB already ships a **Python** MCP SSE bridge (`tools/yardb_mcp_sse.py`) using the official `mcp` SDK:

- `GET /sse` → SSE stream; SDK sends an `endpoint` event pointing at `POST /messages/?session_id=…`  
- `POST /messages/` → JSON-RPC into the session  
- DNS-rebinding protection via `TransportSecuritySettings` (Host/Origin allowlists)  
- Wildcard bind refused (bridge holds `YARDB_PAT` with no client auth)

Goal for net4cpp: provide a **native C++** transport (+ minimal protocol loop) so apps register tools without Python/uvicorn.

### MCP transports to support

| Transport | Priority | Role |
|-----------|----------|------|
| **HTTP+SSE** (legacy MCP SSE) | **P0** after SSE | Matches Cursor / current YarDB config (`type: sse`, URL `…/sse`) |
| **Streamable HTTP** | P1 | Newer MCP HTTP transport; design headers/session so we can add without breaking SSE |
| **stdio** | P2 / elsewhere | Not an `http::server` concern |

Primary reference: [Model Context Protocol](https://modelcontextprotocol.io/) + behavior of `mcp.server.sse.SseServerTransport` (endpoint event, session map, POST fan-in).

### Layering

```text
┌─────────────────────────────────────────┐
│ Application (YarDB / other)             │
│  - tool handlers (health, query, …)     │
├─────────────────────────────────────────┤
│ net MCP protocol (optional module)      │
│  - initialize / tools/list / tools/call │
│  - session table, JSON-RPC 2.0          │
├─────────────────────────────────────────┤
│ net SSE + HTTP POST routes              │
│  - GET /sse  → sse::session             │
│  - POST /messages/ → enqueue to session │
└─────────────────────────────────────────┘
```

Keep **tool business logic out of net**. Net owns framing, sessions, security defaults; apps supply `list_tools` / `call_tool` callbacks (same split as Python `Server` + transport).

### MCP SSE v1 behavior

1. **Session create** on `GET` SSE accept: generate `session_id` (use existing `net:uuid`).  
2. Emit SSE event `event: endpoint` with data = relative POST URI including `session_id` (as SDK expects).  
3. Concurrent `POST /messages/?session_id=` bodies are JSON-RPC messages; validate Content-Type; size-cap with existing body limits.  
4. Responses/notifications go out as SSE `event: message` with JSON payload.  
5. Tear down session on SSE disconnect; POSTs to unknown/expired session → `404`/`409`.  
6. **Security (mandatory for any non-stdio MCP in-process with secrets):**  
   - Default bind guidance: loopback (document; optionally refuse `0.0.0.0`/`::` for MCP helper like YarDB bridge).  
   - Enable Host/Origin allowlists (port FastMCP-style defaults into C++, or document that apps must put MCP behind a trusted proxy).  
   - Do **not** invent a second PAT system in net; apps forward auth as today.

### JSON-RPC surface (v1)

Minimum for Cursor-like clients:

- `initialize` / `notifications/initialized` / `ping`  
- `tools/list` / `tools/call`  
- Structured tool result as text content (compat with current YarDB tools)

Deferred: resources, prompts, sampling, elicitation.

### Tests (MCP)

- Unit: JSON-RPC dispatch table; unknown method; tool error → `isError` content.  
- Integration: fake client opens SSE, reads `endpoint`, POSTs `tools/list`, receives SSE message.  
- Security: evil `Origin` rejected; wildcard bind policy if enforced in helper.  
- Concurrency: two sessions isolated; POST after SSE close fails cleanly.

### Adoption path (YarDB, later)

1. net lands SSE + MCP transport.  
2. YarDB adds **native C++** tool registration first (engine/HTTP semantics as MCP tools).  
3. Keep Python `tools/*_mcp.py` (stdio + SSE) as a **reference client/server** and smoke oracle until native parity is proven — do not delete early.  
4. `.cursor/mcp.json` can point at native `http://127.0.0.1:…/sse` once ready; Python remains available for comparison.

**Exit criteria Phase 2:** MCP SSE session works against a reference client (Python `mcp` SSE client or Cursor); `[net]` tests cover transport; security defaults documented.

---

## Cross-cutting design decisions

### 1. Buffered HTTP vs streaming

Keep existing `callback` API stable. Add parallel registration (`sse` / later `mcp`) that never goes through `Content-Length` body write. Avoid overloading `response_with_headers` with magic sentinel bodies.

### 2. Chunked transfer encoding

- **SSE v1:** omit `Content-Length`, stream until close (widely used). **No chunked response TE yet** (decision #3).  
- **Later:** optional `Transfer-Encoding: chunked` only if a supported proxy requires it.  
- **Requests:** keep rejecting chunked request bodies until a dedicated project decides otherwise (current fail-closed stance is good).

### 3. Threading model

Today: one thread per accepted connection. SSE fits that model (handler blocks in session loop). MCP POSTs arrive on **other** connections/threads and must synchronize into the session’s write path (`std::mutex` per session, or a bounded queue drained by the SSE thread). Document that MCP write affinity stays on the SSE connection thread.

### 4. Middleware

- **CORS:** reuse `cors_middleware` for SSE open and MCP `POST` (no parallel allowlist).  
- Apply other auth checks **at SSE open** and on MCP POSTs explicitly where needed.  
- Metrics: count stream opens/closes; do not treat multi-hour streams as a single “request duration” histogram without care (use separate SSE metrics or cap).

### 5. Module layout (decided)

| Module | Responsibility |
|--------|----------------|
| `net:sse` | Framing + `session` writer |
| `net:http_server` | `server.sse(path).sse(cb)` registration + takeover wiring |
| `net:mcp` (new, in-tree) | Sessions, JSON-RPC, tool callbacks, Host/Origin helpers — **start in net**; extract to a separate repo only if it grows beyond transport/dispatch |
| App (YarDB) | Tool implementations; Python `*_mcp.py` kept as reference |

Follow existing snake_case / modules-only / Allman style; co-locate `*.test.c++`.

---

## Milestones and sequencing

| Milestone | Deliverable | Depends on |
|-----------|-------------|------------|
| **M0** | This plan on `feature/sse-mcp-server` | — |
| **M1** | `net:sse` writer + unit framing tests | M0 |
| **M2** | `http::server` SSE route + integration test | M1 |
| **M3** | Heartbeats, `Last-Event-ID`, docs/README | M2 — **done** (shipped with M1–M2) |
| **M4** | MCP session table + `endpoint` event + POST path | M2 — **done** on `feature/mcp-sse-transport` |
| **M5** | JSON-RPC `initialize` / `tools/*` + security defaults | M4 |
| **M6** | YarDB native MCP spike + keep Python `*_mcp.py` as reference smoke/oracle | M5 |

Do not start M4 until M2 is merged or clearly stable: MCP debugging on a half-baked stream is expensive.

---

## Risks

| Risk | Mitigation |
|------|------------|
| Proxies buffer SSE | Document `X-Accel-Buffering: no`; flush after each event |
| Slow consumers block accept threads | v1 accept; later optional write timeouts / max sessions |
| MCP protocol drift vs SDK | Lock tests to observed `mcp` SSE client behavior; track Streamable HTTP separately |
| Security regression (open bind + tools) | Default loopback docs; Origin/Host checks; no secrets in net itself |
| Scope creep into full agent runtime | Net = transport + dispatch; apps own tools |

---

## Decisions (resolved 2026-07-25)

| # | Question | Decision |
|---|----------|----------|
| 1 | SSE registration API | **`server.sse(path).sse(cb)`** — WebSocket-like, not `get(path).sse(cb)` |
| 2 | CORS | **Reuse `cors_middleware`** — same policy for SSE open / MCP POST; no dedicated parallel allowlist |
| 3 | Chunked response `Transfer-Encoding` | **Not yet** — v1 streams without `Content-Length` until a proxy requirement appears |
| 4 | Where MCP lives | **Start as `net:mcp` in net4cpp**; extract only if the module outgrows transport/dispatch |
| 5 | YarDB adoption | **Native C++ MCP first**; keep Python `*_mcp.py` (stdio + SSE) as **reference** / oracle, not deleted early |

---

## Open questions (remaining)

None for M1–M2 kickoff. Revisit chunked TE (decision #3) only if a concrete reverse-proxy failure shows up in integration.

---

## References

- net4cpp WebSocket spike: `README.md` (“WebSocket (v1 spike)”), `net:websocket`, upgrade path in `net-http_server.c++m`  
- YarDB Python MCP SSE: `tools/yardb_mcp_sse.py`, `tests/mcp/smoke.sh` (evil Origin / wildcard bind)  
- MCP overview: https://modelcontextprotocol.io/  
- SSE: https://html.spec.whatwg.org/multipage/server-sent-events.html  

---

## Next step

Implement **M5** on `feature/mcp-sse-transport`: JSON-RPC `initialize` / `ping` / `tools/list` / `tools/call` + Host/Origin security defaults.
