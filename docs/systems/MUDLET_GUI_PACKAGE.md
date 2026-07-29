# Mudlet GUI Package Auto-Download

## Overview

When a Mudlet client connects, LuminariMUD can push a GMCP message that tells
Mudlet to download and install (or update) a custom GUI package - health/mana/
movement bars, clickable buttons, a minimap, combat panels, etc. Mudlet handles
the download and install itself; the server only advertises a version and URL.

This document traces the full server-side wiring. For the broader protocol
layer (MSDP/GMCP/MSSP/MXP), see [PROTOCOL_SYSTEMS.md](PROTOCOL_SYSTEMS.md).

## The mechanism in one line

Server sends GMCP `Client.GUI` with a `{version, url}` payload; Mudlet compares
the version to what it has installed and downloads the URL if newer.

## Components

### 1. The package definition (compile-time)

`src/protocol.h:216`

```c
#define MUDLET_PACKAGE \
  "{\"version\":4,\"url\":\"https://luminarimud.com/download/LuminariGUI-v2.0.4.015.mpackage\"}"
```

- Payload is JSON: `version` is an integer Mudlet uses to decide whether to
  re-download; `url` is the hosted `.mpackage`.
- Bump `version` whenever a new package should be pushed to already-installed
  clients. The `url` filename carries its own build tag independent of the
  `version` integer.
- Every code path that references the package is wrapped in
  `#ifdef MUDLET_PACKAGE`, so undefining it removes the feature at compile time.
- The header comment above the define claims it is "only defined for the default
  LuminariMUD campaign," but the define is currently unconditional (no
  `#if !defined(CAMPAIGN_*)` guard). Treat the comment as intent, not fact.
- Package hosting is external (luminarimud.com); this repo does not contain the
  `.mpackage` build.

### 2. The runtime toggle

The GMCP-negotiation delivery path is gated by an admin-settable flag.

| Aspect            | Location                                                         |
| ----------------- | --------------------------------------------------------------- |
| Macro             | `CONFIG_AUTO_DL_MUDLET_PACKAGE` -> `config_info.extra.auto_dl_mudlet_package` (`src/utils.h:2749`) |
| Storage field     | `ubyte auto_dl_mudlet_package` in `struct` (`src/structs.h:7846`) |
| Default           | `0` (No) - set in `init_config()` (`src/db.c:7536`)             |
| Options table     | `auto_dl_mudlet_package_options[] = {"No", "Yes"}` (`src/constants.c:5022`) |
| Global default    | `int auto_dl_mudlet_package = 0;` (`src/config.c:392`)          |

### 3. Persistence (config file)

The flag lives in the game config file `lib/etc/config` (`CONFIG_FILE` =
`etc/config` under the `lib` default dir).

- Load: `load_config()` reads the `auto_dl_mudlet_package = <n>` tag and assigns
  `CONFIG_AUTO_DL_MUDLET_PACKAGE` (`src/db.c:7586`). If the tag is absent, the
  default `0` from `init_config()` stands.
- Save: `cedit_save_internally()` writes the tag back
  (`src/cedit.c:1013`).

### 4. In-game admin editing (cedit)

The flag is exposed in the OLC config editor (`cedit`):

- Menu entry: option `M`, "Auto-Download MUDlet Package?" (`src/cedit.c:1371`).
- Selecting it enters edit mode `CEDIT_SET_AUTO_DL_MUDLET_PACKAGE`
  (`src/oasis.h:605`), which lists the No/Yes options (`src/cedit.c:2288`) and
  stores the choice (`src/cedit.c:3507`).
- On save, the value flows back into `CONFIG_AUTO_DL_MUDLET_PACKAGE` and out to
  `lib/etc/config`.

## Delivery: when the package is actually sent

All sends go through `SendGMCP()` (`src/protocol.c:3189`, `#ifdef
MUDLET_PACKAGE`), which frames the payload as
`IAC SB GMCP Client.GUI <json> IAC SE` and writes it only if the descriptor has
GMCP enabled (`pProtocol->bGMCP`).

There are **two live send points**, plus one dead legacy path:

### Path A - GMCP negotiation (config-gated)

`PerformNegotiation()`, GMCP `WILL` handler (`src/protocol.c:2276`).

1. Client sends `IAC WILL GMCP`.
2. If `CONFIG_AUTO_DL_MUDLET_PACKAGE` is on, GMCP is force-enabled for this
   descriptor.
3. If the MSDP `CLIENT_ID` matches `"Mudlet"` (`MatchString`), the package is
   sent via `SendGMCP(apDescriptor, "Client.GUI", MUDLET_PACKAGE)`
   (`src/protocol.c:2291`).

This is the path the admin toggle controls.

### Path B - TTYPE client identification

`PerformSubnegotiation()`, TTYPE handler (`src/protocol.c:2563`).

- When the client name is first learned via TTYPE and it matches `"Mudlet"`,
  GMCP is already enabled, and `CONFIG_AUTO_DL_MUDLET_PACKAGE` is on, the package
  is sent immediately (`src/protocol.c:2568`).
- This path covers the case where Mudlet is identified via terminal type rather
  than via the MSDP `CLIENT_ID` used by Path A. Like Path A, it respects the
  admin toggle, so setting `Auto-Download MUDlet Package?` to No disables both
  send paths at runtime.

### Dead path - legacy PerformHandshake

`src/protocol.c:2334`-`2525` is a commented-out legacy `PerformHandshake()`
function that contains a third `SendGMCP(... "Client.GUI" ...)` call (the
`[DUPLICATE]` debug lines around `src/protocol.c:2506`). It is inside a block
comment and is **not compiled** - do not treat it as a live send point.

## Client side (Mudlet)

Mudlet's GMCP handler recognizes `Client.GUI` with a `{version, url}` object,
compares `version` against the installed package, and downloads/installs from
`url` when newer - prompting the user per Mudlet's own install flow. No further
server involvement is required.

## Quick reference: flow summary

```
compile: #ifdef MUDLET_PACKAGE  (protocol.h:216, JSON version+url)
   |
runtime toggle: CONFIG_AUTO_DL_MUDLET_PACKAGE   (lib/etc/config, cedit option M)
   |
client connects (Mudlet)
   |
   +-- Path A: GMCP WILL + config ON + CLIENT_ID=Mudlet  -> SendGMCP Client.GUI
   +-- Path B: TTYPE=Mudlet + GMCP on + config ON        -> SendGMCP Client.GUI
   |
SendGMCP(): IAC SB GMCP "Client.GUI {version,url}" IAC SE   (protocol.c:3189)
   |
Mudlet downloads/installs the .mpackage if version is newer
```

## Operational notes

- To ship a new GUI build to existing users: host the new `.mpackage`, update
  both the `url` and (if you want auto-update) the `version` integer in
  `src/protocol.h:216`, and recompile.
- To enable at runtime: set `Auto-Download MUDlet Package?` to `Yes` in `cedit`
  (option `M`), or add `auto_dl_mudlet_package = 1` to `lib/etc/config`. Both
  send paths (A and B) respect this toggle.
- To disable entirely: undefine `MUDLET_PACKAGE` and recompile, which removes
  both send paths.
