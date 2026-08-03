# Mudlet GUI Package Auto-Download

## Overview

When a Mudlet client connects, LuminariMUD can send a GMCP `Client.GUI`
advertisement for the custom GUI package. Mudlet performs the download and
installation; the server supplies only an update token and a URL.

Package identity is part of this protocol. Mudlet derives that identity from
the URL filename, so the advertised filename must remain
`LuminariGUI.mpackage` across every release. Put release information in the
`version` field and in the package metadata, not in the advertised filename.

For the broader protocol layer (MSDP/GMCP/MSSP/MXP), see
[PROTOCOL_SYSTEMS.md](PROTOCOL_SYSTEMS.md).

## Identity and update contract

The server sends:

```json
{
  "version": "5",
  "url": "https://luminarimud.com/download/LuminariGUI.mpackage"
}
```

Mudlet handles this advertisement in the following order:

1. It ignores the advertisement if the profile does not allow server-provided
   packages.
2. It derives the package identity from the final path component in `url`.
3. If that package identity is not installed, it downloads and installs it.
4. If the package is installed but Mudlet's stored server-package version is
   different, it replaces the package with the advertised build.
5. If both package identity and stored version match, it does nothing.

The version is an opaque equality token, not a numeric ordering operation. Any
change tells Mudlet to update. It is encoded as a JSON string because string
versions work on older Mudlet releases; integer JSON versions require Mudlet
4.18 or newer.

Mudlet's protocol documentation also requires the URL filename to match the
package name:

- <https://wiki.mudlet.org/w/Manual:GMCP_Extensions>
- <https://wiki.mudlet.org/w/Manual:Package_Manager>

## Package definition in the server

`src/net/protocol.h` defines the payload at compile time:

```c
#define MUDLET_PACKAGE \
  "{\"version\":\"5\",\"url\":\"https://luminarimud.com/download/LuminariGUI.mpackage\"}"
```

The two fields have separate responsibilities:

- `version`: Change this token only when all clients managed through
  `Client.GUI` should install a new build.
- `url`: Keep this at a stable URL whose basename is exactly
  `LuminariGUI.mpackage`.

Do not add a release suffix or query string to the advertised URL. Names such
as `LuminariGUI-v2.0.4.037.mpackage` are different package identities to Mudlet
and can cause repeated downloads, parallel package installs, or failed cleanup
of the previous package.

Every code path that references the package is wrapped in
`#ifdef MUDLET_PACKAGE`. Undefining it removes package advertising at compile
time. The `.mpackage` itself is hosted externally and is not stored in this
repository.

## Package authoring requirements

The exported archive and the server advertisement must agree on one stable
identity:

- Export the Mudlet package with the package name `LuminariGUI`.
- Name the web-served artifact `LuminariGUI.mpackage`.
- Set `mpackage = "LuminariGUI"` in `config.lua`.
- Set `title = "LuminariGUI"` in `config.lua`.
- Set the package metadata version to the GUI release, currently
  `2.0.4.037`.
- Ensure exported XML package ownership uses `LuminariGUI` where Mudlet's
  exporter supplies a package name.

The versioned build may also be retained on the web server as an archive or
manual-download alias, but `Client.GUI` must advertise the stable filename.

## Runtime toggle

The delivery paths are gated by an administrator-controlled flag.

| Aspect | Location |
| --- | --- |
| Macro | `CONFIG_AUTO_DL_MUDLET_PACKAGE` in `src/utils.h` |
| Storage field | `auto_dl_mudlet_package` in `struct config_data` |
| Default | `0` (`No`) in `init_config()` in `src/db.c` |
| Options | `auto_dl_mudlet_package_options[] = {"No", "Yes"}` in `src/constants.c` |
| Config tag | `auto_dl_mudlet_package = 0` or `1` in `lib/etc/config` |

`load_config()` reads the tag. If it is absent, the default remains `0`.
`cedit_save_internally()` writes the selected value back to the config file.

The flag is also exposed through `cedit`:

- Menu option `M` is `Auto-Download MUDlet Package?`.
- Selecting it offers `No` and `Yes`.
- Saving updates `CONFIG_AUTO_DL_MUDLET_PACKAGE` and `lib/etc/config`.

Setting this option to `No` stops both live delivery paths for every player. It
does not selectively suppress delivery for one profile.

## Delivery paths

All sends use `SendGMCP()` in `src/net/protocol.c`, which emits
`IAC SB GMCP Client.GUI <json> IAC SE` only when GMCP is enabled.

There are two live send points:

### Path A - GMCP negotiation

In the GMCP `WILL` handler in `PerformNegotiation()`:

1. The client sends `IAC WILL GMCP`.
2. The server enables GMCP for the descriptor.
3. If automatic package delivery is enabled and the client ID matches Mudlet,
   the server sends `Client.GUI`.

### Path B - TTYPE client identification

In the TTYPE handler in `PerformSubnegotiation()`:

- When the first terminal type identifies Mudlet, the server sends
  `Client.GUI` if GMCP and automatic package delivery are both enabled.
- This covers negotiation orders where Mudlet is identified through TTYPE
  after GMCP is already active.

A commented-out legacy `PerformHandshake()` implementation also contains a
`Client.GUI` call. It is inside a block comment and is not compiled.

## What an existing installation does

| Profile state | Result after receiving the advertisement |
| --- | --- |
| `LuminariGUI` is absent | Mudlet downloads and installs it |
| `LuminariGUI` is present and stored server version is `5` | No download |
| Present, but stored server version differs | Mudlet replaces it with the advertised build |
| Server-provided packages are disabled in the profile | Mudlet ignores the advertisement |

Mudlet persists the server-managed package name and version in the profile.
Consequently, packages previously installed through this same stable
`Client.GUI` identity do not download again on every connection.

### Manually installed packages

A manual installation can have the correct `LuminariGUI` package name but lack
Mudlet's private stored server-package version. In that case Mudlet can perform
one server-managed replacement when it first receives version `5`. After that
replacement, subsequent connections with the same identity and version do not
download again.

The MUD server cannot query Mudlet's installed package list through this
protocol, so it cannot reliably distinguish a manual install from an absent or
older server-managed install.

To guarantee that a particular profile never accepts the automatic package,
the player must turn off `Allow server to install script packages` in that
profile's Mudlet preferences. This is a client-side opt-out and also blocks
future server-managed GUI updates.

## One-time migration from versioned filenames

Older LuminariMUD builds advertised a filename such as
`LuminariGUI-v2.0.4.015.mpackage`. Mudlet may have recorded that basename as the
server package identity. Moving to `LuminariGUI.mpackage` can therefore cause
one final download even when an older GUI is present.

After the stable package has been installed with server version `5`, reconnects
do not download it again until the version token changes. If a profile shows
both the stable package and a legacy version-suffixed package in Mudlet's
Package Manager, remove the legacy entry and keep `LuminariGUI`.

Do not alternate between stable and versioned advertised filenames after this
migration; doing so creates a new identity each time.

## Web-server deployment checklist

The web server's download directory should present this layout:

```text
download/
|-- LuminariGUI.mpackage                  required Client.GUI target
`-- LuminariGUI-v2.0.4.037.mpackage       optional archival copy
```

For each GUI release:

1. Export and validate the new archive with package identity `LuminariGUI`.
2. Optionally store a versioned archival copy.
3. Replace `LuminariGUI.mpackage` atomically with the new archive. Do not expose
   a partially uploaded file at the stable path.
4. Keep file permissions readable by the web server.
5. Ensure the advertised URL returns the package bytes, not an HTML download
   page or access-denied response.
6. If HTTP redirects to HTTPS or to `www`, retain the
   `LuminariGUI.mpackage` basename through the redirect chain.
7. Purge or revalidate any CDN cache for the stable URL.
8. Only after the stable URL serves the new archive, change the server's GMCP
   version token and deploy the MUD binary.

Example validation from the download directory:

```bash
unzip -t LuminariGUI.mpackage
unzip -p LuminariGUI.mpackage config.lua | grep 'mpackage = "LuminariGUI"'
curl --fail --location --output /tmp/LuminariGUI.mpackage \
  https://luminarimud.com/download/LuminariGUI.mpackage
unzip -t /tmp/LuminariGUI.mpackage
```

## Operational summary

- To publish a GUI update: replace the bytes at the stable URL, then change the
  `version` string in `src/net/protocol.h` and recompile.
- To enable advertising: set `Auto-Download MUDlet Package?` to `Yes` in
  `cedit` option `M`, or set `auto_dl_mudlet_package = 1` in `lib/etc/config`.
- To disable advertising globally at runtime: set that option to `No`.
- To remove the feature at compile time: undefine `MUDLET_PACKAGE` and rebuild.
- To opt out for one player profile: disable server-provided packages in
  Mudlet's profile preferences.

## Flow summary

```text
MUDLET_PACKAGE: version "5" + stable LuminariGUI.mpackage URL
  -> runtime auto-download option enabled
  -> Mudlet identified and GMCP enabled
  -> server sends Client.GUI
  -> Mudlet compares URL-derived identity and stored server version
     |-- missing identity: download and install
     |-- different version: replace package
     `-- same identity and version: no download
```
