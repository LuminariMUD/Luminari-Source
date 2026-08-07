# Operational API Contracts

## Loopback Health API

The existing Terrain API listener exposes HTTP health routes for local
operators, CI, and systemd. It binds to `127.0.0.1` only and shares the
single-threaded game loop. It is not a public application API and provides no
remote authentication or TLS termination.

Default base URL: `http://127.0.0.1:8182`

Set `TERRAIN_API_PORT` to an integer from 1025 through 65535 before starting the
server to use another listener port.

### Routes

| Method | Path | Success | Meaning |
|--------|------|---------|---------|
| `GET`, `HEAD` | `/health` | 200 | Game loop is serving and MariaDB is reachable |
| `GET`, `HEAD` | `/health/ready` | 200 | Alias of `/health` |
| `GET`, `HEAD` | `/health/live` | 200 | Initialized game loop is serving; database is not checked |

Readiness returns 503 when MariaDB is unavailable. Unknown routes return 404,
unsupported HTTP methods return 405, and malformed HTTP/1.0 or HTTP/1.1
requests return 400. Responses are JSON, set `Cache-Control: no-store`, and
close the connection. `HEAD` returns the same status and headers without a
body.

Readiness body:

```json
{"service":"luminari-mud","status":"healthy","database":"healthy","uptime_seconds":42}
```

Degraded readiness changes both `status` and `database` to `unhealthy`.
Liveness reports `database` as `not_checked`.

### Operator Probe

Use the maintained fail-closed client instead of parsing JSON in service
configuration:

```bash
./scripts/operations/healthcheck.sh
./scripts/operations/healthcheck.sh --wait
```

| Variable | Default | Purpose |
|----------|---------|---------|
| `LUMINARI_HEALTH_URL` | `http://127.0.0.1:8182/health` | Readiness URL |
| `LUMINARI_HEALTH_REQUEST_TIMEOUT_SECONDS` | `3` | Per-request curl timeout |
| `LUMINARI_HEALTH_TIMEOUT_SECONDS` | `90` | Total `--wait` deadline |
| `LUMINARI_HEALTH_INTERVAL_SECONDS` | `2` | Delay between wait attempts |

All timeout values must be positive integers. The script requires `curl` and
accepts readiness only when the response contains the expected service,
healthy status, and healthy database fields.

### Security and Performance Boundary

Keep the listener loopback-only. Readiness performs a synchronous MariaDB
connectivity check in the main game loop, so probes must remain bounded and
should not be sent at high frequency. Use `/health/live` when database state is
not part of the decision.

Implementation: `src/wilderness/terrain_bridge.c`. Regression coverage:
`unittests/CuTest/test_gameplay_e2e.c` and
`scripts/operations/test_healthcheck.sh`.

## Other Protocols

Telnet option negotiation, MSDP, GMCP, and related game-client protocols are
documented in [Protocol Systems](../systems/PROTOCOL_SYSTEMS.md). The separate
InterMUD3 JSON-RPC contract is documented in
[InterMUD3 Gateway API](../systems/INTERMUD3_GATEWAY_API.md).
