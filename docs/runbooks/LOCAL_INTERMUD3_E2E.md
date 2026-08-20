# Local LuminariMUD and Intermud3 End-to-End Runbook

This runbook starts the local LuminariMUD server, the local Intermud3 gateway,
and the gateway's connection to the public I3 router. It is intended for the
development checkout at:

- `/home/aiwithapex/projects/Luminari-Source`
- `/home/aiwithapex/projects/Intermud3`

Do not use this procedure against production. Confirm that
`Luminari-Source/lib/.env` contains `APP_ENV=development` before making changes.
Do not modify `lib/.env`, `lib/mysql_config`, or commit credentials.

## Data flow

```text
MUD players
    |
LuminariMUD :4100
    |
JSON-RPC/TCP :8081
    |
local Intermud3 gateway
    |
public Intermud-3 router
```

The gateway HTTP health endpoint is on port 8080. MariaDB must also be running
for LuminariMUD to boot.

## One-time configuration

Configure the gateway's ignored `.env` from `.env.example`. At minimum, verify
the MUD identity, public router, local API ports, and the Luminari API key:

```text
MUD_NAME=LuminariMUD
MUD_PORT=4100
I3_ROUTER_HOST=<public-router-host>
I3_ROUTER_PORT=<public-router-port>
API_PORT=8080
API_KEY_LUMINARI=<local-shared-key>
```

The bundled gateway configuration exposes its TCP API on port 8081.

If `lib/i3_config` does not exist, copy `lib/i3_config.example` to it. Configure
the real ignored file with the local gateway address and the same shared key:

```text
gateway_host 127.0.0.1
gateway_port 8081
I3_API_KEY=<local-shared-key>
I3_MUD_NAME=LuminariMUD
default_channel intermud
auto_reconnect 1
reconnect_delay 30
```

Protect the real client configuration:

```bash
chmod 600 /home/aiwithapex/projects/Luminari-Source/lib/i3_config
```

Never put a real key in `lib/i3_config.example`, documentation, test output, or
logs.

## Build and test

Build and install the MUD before starting it. `make install` is required after
`make test`; it installs `bin/luminari` and removes the temporary root-level
`luminari` binary.

```bash
cd /home/aiwithapex/projects/Luminari-Source
make clean
make -j"$(nproc)"
make test
make install
```

Run the focused gateway protocol and API regressions:

```bash
cd /home/aiwithapex/projects/Intermud3
venv/bin/python -m pytest \
  tests/unit/test_live_protocol_state.py \
  tests/unit/api/test_event_bridge.py \
  tests/unit/api/test_session.py \
  tests/api/test_events.py \
  tests/unit/network/test_mudmode.py \
  -q --no-cov
```

## Start supervised local services

Start the gateway first:

```bash
systemd-run --user \
  --unit=i3-gateway-local \
  --collect \
  --description="Local Intermud3 Gateway" \
  --property=WorkingDirectory=/home/aiwithapex/projects/Intermud3 \
  --property=Restart=on-failure \
  --property=RestartSec=5s \
  /home/aiwithapex/projects/Intermud3/venv/bin/python \
  -m src --log-level INFO
```

Then start the MUD:

```bash
systemd-run --user \
  --unit=luminari-local \
  --collect \
  --description="Local LuminariMUD" \
  --property=WorkingDirectory=/home/aiwithapex/projects/Luminari-Source \
  --property=Restart=on-failure \
  --property=RestartSec=5s \
  --property=TimeoutStopSec=20s \
  /home/aiwithapex/projects/Luminari-Source/bin/luminari -d lib 4100
```

These are transient user units. They remain supervised for the current user
service manager, but they are not a replacement for installed production unit
files.

## Verify the complete path

Check service state and listening ports:

```bash
systemctl --user status i3-gateway-local.service --no-pager
systemctl --user status luminari-local.service --no-pager
ss -ltnp '( sport = :4100 or sport = :8080 or sport = :8081 )'
curl -fsS http://127.0.0.1:8080/health
```

The health response should report `status` as `healthy` and normally show one
active session for the MUD.

Inspect the gateway:

```bash
journalctl --user -u i3-gateway-local.service -f
```

Look for the public router connection reaching `ready` and a TCP session
authenticating as `LuminariMUD`.

Inspect the MUD:

```bash
journalctl --user -u luminari-local.service -f
```

Look for all of the following:

```text
Config loaded - Host: 127.0.0.1, Port: 8081, MUD: LuminariMUD
Successfully authenticated with I3 gateway
Updated local MUD list with <count> entries
Updated local channel list with <count> entries
```

Counts vary with live network state. The MUD client supports fragmented
snapshots larger than 4 KB and wakes the idle game loop when I3 events arrive,
so these updates and incoming tells work even with zero players connected.

Connect a MUD client to `127.0.0.1:4100`. Useful player checks are:

```text
i3mudlist
i3channels
i3who <mud-name>
i3finger <user>@<mud-name>
i3locate <user>
i3tell <user>@<mud-name> <message>
i3chat [channel] <message>
```

Immortals can inspect `i3admin status` and `i3admin stats`.

For a true network test, use a cooperating remote MUD or a second authenticated
gateway client to send a uniquely identifiable tell through the public router.
Confirm the gateway receives `tell_received` and that the MUD logs delivery (or
`player not found` for an intentionally nonexistent local user).

## Restart and stop

Code changes require restarting the corresponding service:

```bash
systemctl --user restart i3-gateway-local.service
systemctl --user restart luminari-local.service
```

The MUD automatically reconnects after a gateway restart. The configured
`reconnect_delay` controls how long that takes.

Stop both services cleanly:

```bash
systemctl --user stop luminari-local.service
systemctl --user stop i3-gateway-local.service
```

## Troubleshooting

- `Connection refused` on 8081: start the gateway first and verify its TCP
  listener.
- Authentication failure: confirm the MUD key and gateway
  `API_KEY_LUMINARI` value match. Do not print either value.
- Parameterless `mudlist` or `channel_list` returns a `NoneType` error: the
  gateway is running an old build; restart it from the current checkout.
- The MUD receives a large snapshot but does not update its cache: confirm the
  running executable is the current `bin/luminari`, then check for framing or
  oversized-response errors.
- Incoming events appear only after a player connects: the running MUD predates
  the idle main-loop wake fix; rebuild, install, and restart it.
- The gateway is healthy but not globally connected: inspect its router state.
  Local API health alone does not prove the public router is `ready`.
- A gateway restart can take up to `reconnect_delay` seconds to appear in MUD
  logs.

Debug builds must not log raw TCP payloads because authentication messages
contain API keys. If a key was ever exposed in historical logs, rotate it in
both ignored configurations.
