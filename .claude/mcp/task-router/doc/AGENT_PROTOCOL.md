# Task Router Agent Protocol

**Version:** v4.0 (2026-06-01)
**Status:** Phase A of the agent-agnostic plan (see `memory/project_agent_agnostic_direction.md`).
**Audience:** anyone building a non-Claude-Code agent that needs to register with the task-router and participate in dispatch — Anthropic SDK scripts, OpenAI Assistants, LangGraph, custom in-house agents, Node tools, Python tools.

## Abstract

**TL;DR:** Framework-neutral runtime contract for the task-router's MCP + REST surface. Describes the lifecycle (`pickup_next_task → … → complete_task`), error-recovery patterns, memory primitives, registration, auth, and the JSON-RPC over Streamable HTTP transport — without assuming Claude Code, the VS Code extension, or any specific client framework. The canonical seeded Node client (`.claude/mcp/task-router/client.js`) is one valid implementation of this contract; a reference Python snippet is included for non-Node consumers.

**Load when:** designing or implementing a new agent type; integrating a non-Claude framework; debugging "MCP unavailable" / 406 / "Server not initialized" errors; building cross-machine PM-to-PM federation.

**Key facts:**
- Transport is **MCP Streamable HTTP** at `POST /mcp?project=<slug>`. Native MCP tools are exposed only when a client integrates the MCP SDK; raw HTTP works for everything else.
- A read-only REST surface (`/stats`, `/tasks`) exists alongside MCP for status queries.
- All tools require both `project` and `agent` arguments; the URL `?project=` selects the tenant.
- Errors are runtime-authoritative — the response body names the recovery tool, per the v0.8.6 pattern.
- The seeded `client.js` ships with every project; reuse it before reaching for a custom transport.

**Owner:** task-router maintainers (this doc lives in `doc/seed/` and ships with every consumer).
**Related:** `doc/reference/SERVER_API.md` (REST + MCP tool catalog), `doc/runbooks/MAJORDOMUS_RUNBOOK.md` (federation), `doc/runbooks/TROUBLESHOOTING.md`.

---

## 1. Transport

### 1.1 Endpoint

```
POST http://127.0.0.1:3100/mcp?project=<project-slug>
```

The `?project=` parameter is **required** on every call (not just session init) so a single server can host multiple tenants in one process. The server returns `{"error": "missing_project_param", ...}` with a migration link if it's missing.

### 1.2 Required headers

```
Content-Type: application/json
Accept: application/json, text/event-stream
```

The `Accept` clause is **not optional**. Standard JSON clients send only `application/json` and receive HTTP 406 from the MCP layer. This is the single most common transport mistake.

### 1.3 Session lifecycle

MCP Streamable HTTP is a two-step handshake:

1. **`initialize`** — POST a JSON-RPC `initialize` request. The server replies with status 200 and sets an `mcp-session-id` response header. Capture it.
2. **`tools/call`** — every subsequent call sets the captured value as the `mcp-session-id` request header. Without it the server replies `{"error": "Server not initialized"}`.

Sessions are per-process; persist `sid` for the duration of your agent run.

### 1.4 Response framing (SSE)

Responses come back SSE-framed: one `data: <json>` line per message. Parse the first `data:`-prefixed line and JSON-decode it. Tool results are wrapped:

```
parsed.result.content[0].text   →  the actual tool return value, JSON-encoded again
```

So `JSON.parse(parsed.result.content[0].text)` gives you the structured object. The doubly-encoded shape is correct MCP framing and is here to stay.

### 1.5 Authentication

When `TASK_ROUTER_API_KEY` is set on the server, every request must carry `x-task-router-key: <key>`. The seeded client picks the key up from `TASK_ROUTER_API_KEY` automatically. Servers without a key configured ignore the header.

### 1.6 Client identity (recommended)

Send `x-task-router-client: <id>/<version>` (e.g. `node/v4.0`, `py/v4.0`) so future server versions can log a deprecation warning when they see an outdated client. The seeded `client.js` does this automatically.

---

## 2. Lifecycle

The fleet uses a 2-call lifecycle. Earlier 5-call flows (`check_inbox`, `accept_task`, `submit_result`, `acknowledge_results`) are legacy and remain only for backwards compatibility — new agents should not use them.

### 2.1 Worker (specialist) lifecycle

```
   register_agent          ←  performed mechanically by the launcher (start.sh /
                              extension terminal manager) BEFORE the agent loads
   pickup_next_task        ←  agent's first call. DB-authoritative — returns the
                              next pending task OR a previously accepted task with
                              {resumed: true} after compaction.
   <agent does the work>
   complete_task           ←  agent's last call. Result is a free-form string;
                              for structured results put JSON inside.
```

`pickup_next_task` is the **only** correct way to obtain the in-flight `task_id` after a context compaction. Agents must never confabulate a UUID from partial summary; if you have lost the id, call `pickup_next_task` — it returns the in-flight task with the canonical id.

### 2.2 PM (dispatcher) lifecycle

```
   register_agent          ←  mechanical, as above
   list_agents             ←  inspect the roster
   dispatch_task(to=X, payload=...)
                           ←  pending row created; the target agent picks it up
                              via pickup_next_task on its side
   collect_results(dispatcher="pm")
                           ←  drains completed results that have not yet been
                              acknowledged. One call per turn is enough — do not
                              poll.
```

`dispatch_task` is fire-and-forget. PM does not block on the worker.

### 2.3 Error recovery (runtime-authoritative)

Server errors carry the recovery tool name in the message body. The v0.8.6 pattern: a `complete_task` "not found" error reads `"...call pickup_next_task(agent=X) — it is DB-authoritative and returns your in-progress task with the correct id; then retry."` Agents read these messages and act on them; do not invent recovery paths from training data.

---

## 3. Tool catalog (essentials)

The full catalog is in `doc/reference/SERVER_API.md`. The essential subset:

| Tool | Args | Returns |
|---|---|---|
| `register_agent` | `name`, `project`, `capabilities[]`, optional `metadata`, optional `remote: {url, project, target_agent, api_key_env}` | `{ok: true, agent: {...}}` |
| `pickup_next_task` | `agent`, `project` | `{task: {...}, resumed?: true}` or `{task: null}` |
| `complete_task` | `task_id`, `agent`, `project`, `result` (string) | `{ok: true}` |
| `dispatch_task` | `to`, `from?`, `project`, `payload` (string) | `{task_id}` |
| `collect_results` | `dispatcher`, `project` | `{results: [{task_id, from, result}, ...]}` |
| `list_agents` | `project` | `{agents: [{name, status, ...}, ...]}` |
| `save_memory` / `load_memory` | `agent`, `project`, `key?`, `value?` | per-agent KV; reserved for runtime state like `_in_progress_task` |
| `wait_for_result` | `task_id`, `project`, `timeout_ms?` | long-poll; resolves when the task hits a terminal state |

`task_id` is a UUID. Argument names are **literal** — `to_agent` / `from_agent` / `task_to` are not aliases; the server rejects them.

---

## 4. Memory model

Two distinct storage layers — do not conflate them.

**Server-managed runtime memory (`save_memory` / `load_memory`).** Per-agent key/value store in the server DB. Intended for runtime state like the reserved `_in_progress_task` key that survives terminal restarts. Cheap; not durable across server-DB resets.

**Repo `memory/` folder (PM only).** Version-controlled with the project. PM writes here; specialists do not (see Section 5).

**Per-agent guidelines (`doc/<agent>_GUIDELINES.md`).** Each specialist's single sanctioned durable write target — but only on explicit PM/user request. Indexed in `DOC_OWNERSHIP_MATRIX.md`.

---

## 5. Memory Policy (specialist agents)

Specialists **MUST NOT** create or update auto-memory files. The only durable storage available to a specialist is:

1. `save_memory` / `load_memory` for short-lived runtime state.
2. `doc/<agent>_GUIDELINES.md`, written only on explicit PM/user request.

Never write to `memory/`, `.claude/memory/`, or any harness-managed auto-memory directory. If a stray file appears, report it on the next `/pm audit` and do not modify it.

---

## 6. Registration

The launcher (extension's terminal manager, `start.sh`, or the consumer's equivalent) calls `POST /api/register?project=<slug>` **before** the agent's runtime loads. This makes registration mechanical and removes the "agent forgot to call register_agent" failure class.

For non-Claude consumers integrating from scratch:

```http
POST /api/register?project=<slug> HTTP/1.1
Content-Type: application/json
x-task-router-key: <optional>

{
  "name": "<agent-name>",
  "project": "<slug>",
  "capabilities": ["..."],
  "metadata": { "tier": "specialist" }
}
```

Idempotent on `(name, project)`. Federation peers register a `remote` block — see `doc/runbooks/MAJORDOMUS_RUNBOOK.md`.

---

## 7. Reference implementations

### 7.1 Node (canonical)

`.claude/mcp/task-router/client.js` ships with every consumer. Exposes:

```
node client.js pickup
node client.js complete --task-id=<id> --result='<text>'
node client.js dispatch --to=<agent> --payload='<text>'
node client.js collect-results
node client.js list-agents
```

Stdout schema: `{"ok": true|false, "result": ..., "error": ...}`. Exit code 0 on success, 1 on failure. Reads `TASK_ROUTER_AGENT`, `TASK_ROUTER_PROJECT`, `TASK_ROUTER_BASE_URL`, `TASK_ROUTER_API_KEY` from env.

### 7.2 Python (verified 2026-06-01)

Drop-in stdlib reference; use this when the consumer's runtime is Python and adding a Node dependency is undesirable. From `doc/feedbacks/Jun01_2026/feedback_task_router_rest.md`:

```python
import urllib.request, json

BASE = 'http://127.0.0.1:3100/mcp?project=' + PROJECT

def mcp_init():
    body = {'jsonrpc': '2.0', 'id': 1, 'method': 'initialize',
        'params': {'protocolVersion': '2024-11-05', 'capabilities': {},
                   'clientInfo': {'name': AGENT, 'version': '1.0'}}}
    req = urllib.request.Request(BASE, data=json.dumps(body).encode(),
        headers={'Content-Type': 'application/json',
                 'Accept': 'application/json, text/event-stream'}, method='POST')
    resp = urllib.request.urlopen(req, timeout=5)
    resp.read()
    return resp.headers.get('mcp-session-id')

def mcp_call(req_id, method, params, sid):
    body = {'jsonrpc': '2.0', 'id': req_id, 'method': method, 'params': params}
    req = urllib.request.Request(BASE, data=json.dumps(body).encode(),
        headers={'Content-Type': 'application/json',
                 'Accept': 'application/json, text/event-stream',
                 'mcp-session-id': sid}, method='POST')
    resp = urllib.request.urlopen(req, timeout=5)
    raw = resp.read().decode()
    for line in raw.splitlines():
        if line.startswith('data:'):
            return json.loads(line[5:].strip())

sid = mcp_init()
task = mcp_call(2, 'tools/call', {'name': 'pickup_next_task',
    'arguments': {'agent': AGENT, 'project': PROJECT}}, sid)
# ... do work ...
mcp_call(3, 'tools/call', {'name': 'complete_task',
    'arguments': {'task_id': TASK_ID, 'agent': AGENT,
                  'project': PROJECT, 'result': '...'}}, sid)
```

---

## 8. Federation (Majordomus mode)

For cross-machine PM-of-PMs orchestration:

1. Local PM calls `register_agent` with a `remote: {url, project, target_agent, api_key_env}` block.
2. Local `dispatch_task(to=<remote-name>, …)` triggers `mcp-task-router/src/federation.js#forwardDispatch` which `POST`s `/api/dispatch?project=<remote_project>` on the peer.
3. `wait_for_result` long-polls the peer's `complete_task` back into the local task record.

Full schema and operational guidance: `doc/runbooks/MAJORDOMUS_RUNBOOK.md`.

---

## 9. Versioning

The protocol revision lives in `seed-state.json#seed_version` and is mirrored by every signed erratum's `applies_to_seed_versions` range. The seeded `client.js` declares its version via `--protocol-version` (currently `node/v4.0`).

When the server adds a tool argument, the change is signalled via an erratum whose payload may include an updated `client.js` snapshot. The `/pm sync seed` command applies the seed-file half (`PM_TEMPLATES.md`, `PM.md`); the client-helper half travels alongside.

---

## 10. Anti-patterns

- **Rolling your own HTTP client.** The Jun01 2026 consumer batch documents the failure modes (406, "Server not initialized"); use the canonical client.
- **Polling `pickup_next_task` in a `while True` loop.** The launcher keeps the terminal alive; the inbox check is event-driven via the `UserPromptSubmit` hook. Tight polling burns server CPU and tokens.
- **Confabulating a `task_id` after compaction.** Always call `pickup_next_task` to recover the canonical id.
- **Using `task_to` / `to_agent` / `from_agent` as argument names.** Literal `to` / `from`, no aliases.
- **Writing to `memory/` from a specialist.** PM only. See Section 5.
- **Treating `See doc/...` as optional.** Soft pointers are skipped when an inline attempt fails first. This doc and SKILL.md transport sections use imperative wording for exactly that reason.
