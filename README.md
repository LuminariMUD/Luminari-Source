![LuminariMUD - a moonlit fantasy harbor, mountain wilderness, and an adventurer overlooking the world](docs/images/luminari-readme-header.webp)

<p align="center">
  <a href="https://github.com/LuminariMUD/Luminari-Source/actions/workflows/test.yml"><img src="https://img.shields.io/github/actions/workflow/status/LuminariMUD/Luminari-Source/test.yml?style=flat-square&amp;label=build%20%26%20test" alt="Build and test workflow status"></a>
  <a href="https://github.com/LuminariMUD/Luminari-Source/actions/workflows/quality.yml"><img src="https://img.shields.io/github/actions/workflow/status/LuminariMUD/Luminari-Source/quality.yml?style=flat-square&amp;label=code%20quality" alt="Code quality workflow status"></a>
  <a href="https://github.com/LuminariMUD/Luminari-Source/actions/workflows/security.yml"><img src="https://img.shields.io/github/actions/workflow/status/LuminariMUD/Luminari-Source/security.yml?style=flat-square&amp;label=security" alt="Security workflow status"></a>
  <a href="docs/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md"><img src="https://img.shields.io/badge/docs-explore-d4a853?style=flat-square" alt="Explore the documentation"></a>
</p>
<p align="center">
  <a href="src/"><img src="https://img.shields.io/badge/language-GNU%20C23-527fa8?style=flat-square&amp;logo=c&amp;logoColor=white" alt="Language: GNU C23"></a>
  <a href="sql/"><img src="https://img.shields.io/badge/database-MariaDB%20%2F%20MySQL-267f83?style=flat-square&amp;logo=mariadb&amp;logoColor=white" alt="Database: MariaDB or MySQL"></a>
  <a href="docs/guides/SETUP_AND_BUILD_GUIDE.md"><img src="https://img.shields.io/badge/build-Autotools-527fa8?style=flat-square" alt="Build with Autotools"></a>
  <a href="docs/development/CMAKE_BUILD_GUIDE.md"><img src="https://img.shields.io/badge/build-CMake-527fa8?style=flat-square&amp;logo=cmake&amp;logoColor=white" alt="Build with CMake"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-project%20%2B%20inherited%20terms-d4a853?style=flat-square" alt="License: project and inherited terms"></a>
</p>

# LuminariMUD

LuminariMUD is a text-based multiplayer game server implementing Pathfinder and
D&D 3.5 mechanics on the tbaMUD/CircleMUD foundation. The supported server is
written in GNU C23 and requires MariaDB or MySQL at runtime.

Current version: `2.5062-beta` (tbaMUD 3.64).

## Quick Start

On Ubuntu, Debian, or WSL2, the repository's one-command setup installs
dependencies, prepares local configuration and world data, provisions MariaDB,
builds the server, and installs `bin/luminari`:

```bash
./scripts/deployment/deploy.sh
```

Then start the server:

```bash
./bin/luminari -d lib
```

The checked-in local runtime configuration defaults to the reserved development
port 4101. Connect a MUD client to `localhost:4101`. The production systemd
unit explicitly uses port 4100. The deployment script supports noninteractive,
development, production, and managed-systemd modes; inspect the exact options
with `./scripts/deployment/deploy.sh --help`.

For a fresh clone, start with the [onboarding checklist](docs/onboarding.md) or
the [setup and build guide](docs/guides/SETUP_AND_BUILD_GUIDE.md).

## Development Check

Autotools is the preferred incremental build. The authoritative one-command
test and installation gate is:

```bash
make test && make install
```

`make test` builds the production-linked CuTest suite and shell regressions.
`make install` activates the tested immutable build under `bin/releases/` and
removes the root-level `luminari` artifact.

## Architecture

One select-based game loop connects player commands, scheduled updates, and a
shared world. Game systems run inside the server; DG Scripts add behavior to
rooms, mobiles, and objects, while Oasis OLC lets builders edit the world in game.

```mermaid
flowchart TB
    players["Players and builders<br/>MUD clients"]

    subgraph server["LuminariMUD server | GNU C23"]
        loop["Network and game loop<br/>comm.c"]
        commands["Command dispatch<br/>interpreter.c"]
        ticks["Heartbeat and events<br/>Scheduled world updates"]
        game["Game systems<br/>Characters, combat, magic, quests<br/>Wilderness, vessels, crafting"]
        scripts["DG Scripts<br/>Room, mobile, object triggers"]
        olc["Oasis OLC<br/>In-game world editing"]
    end

    database[("MariaDB / MySQL<br/>Accounts, characters, help<br/>and subsystem data")]
    world[("World files | lib/world/<br/>Rooms, mobiles, objects<br/>Zones, shops, quests, triggers")]

    players <-->|"Commands / text"| loop
    loop --> commands
    loop --> ticks
    commands --> game
    ticks --> game
    commands --> scripts
    ticks --> scripts
    commands --> olc
    game <-->|"Persist / load"| database
    olc -->|"Save authored content"| world
    game ---|"World loaded by db.c"| world

    classDef entry fill:#172c43,stroke:#80b9d8,color:#ffffff
    classDef runtime fill:#173e44,stroke:#6dbcb4,color:#ffffff
    classDef content fill:#3e334b,stroke:#bc9acd,color:#ffffff
    classDef storage fill:#493d28,stroke:#d4a853,color:#ffffff
    class players,loop entry
    class commands,ticks,game runtime
    class scripts,olc content
    class database,world storage
    style server fill:#f0f5f8,stroke:#7893a7,color:#172c43
```

This overview shows the main execution and content paths, rather than every
subsystem dependency. See the [architecture guide](docs/ARCHITECTURE.md) for details.

## Repository Structure

```text
.
|-- src/          # GNU C23 server and game systems
|-- lib/          # Runtime configuration, text, and flat-file world data
|-- sql/          # Master schema and component migrations/verifiers
|-- scripts/      # Deployment, operations, debugging, and world tools
|-- unittests/    # Production-linked CuTest and focused harnesses
`-- docs/         # Maintained developer, builder, and operator documentation
```

MariaDB stores accounts, characters, help, and subsystem data. Flat files under
`lib/world/` remain the authored room, mobile, object, zone, shop, quest, and
trigger sources. The server uses a single select-based game loop; DG Scripts
provide content-local behavior and Oasis OLC provides in-game world editing.

## Documentation

- [Documentation index](docs/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Development commands](docs/development.md)
- [Deployment and CI/CD](docs/deployment.md)
- [Environment boundaries](docs/environments.md)
- [Testing guide](docs/guides/TESTING_GUIDE.md)
- [Incident response](docs/runbooks/incident-response.md)
- [Operational API contracts](docs/api/README_api.md)
- [Contributing](CONTRIBUTING.md)

Player and builder orientation remains in [Getting Started](docs/GETTING_STARTED.md)
and the [builder quickstart](docs/world_game-data/BUILDER_QUICKSTART.md).

## Project Status

The special-procedure architecture initiative completed Phases 00-02: registry
safety and observability, call-site gateways, and validated declarative legacy
assignments. Content extraction, shared mechanics, typed handlers, and
conditional composition remain planned in the
[special-procedure refactor PRD](docs/ongoing-projects/spec-todo.md).

## LuminariMUD Eco-System

What we call the "Lumiverse":

- [Sage GraphRAG lore and world building](https://github.com/LuminariMUD/sage)
- [Luminari web client](https://github.com/LuminariMUD/luminariweb)
- [InterMUD-3 client](https://github.com/LuminariMUD/Intermud3)
- [Discord bridge](https://github.com/LuminariMUD/discord-mud-chat)
- [Wilderness editor](https://github.com/LuminariMUD/wildeditor)
- [Mudlet interface](https://github.com/LuminariMUD/LuminariGUI)

## Contributing and Community

Read [CONTRIBUTING.md](CONTRIBUTING.md) before opening a pull request and
[CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) before participating in project spaces.
Questions and bug reports can be raised through
[GitHub Issues](https://github.com/LuminariMUD/Luminari-Source/issues).

## License

Custom LuminariMUD code is dedicated to the public domain under the Unlicense.
Inherited tbaMUD, CircleMUD, DikuMUD, and licensed game content retain their
respective terms. See [LICENSE](LICENSE) and the
[legal notes](docs/legal/README_legal.md) for the project's complete notice.
