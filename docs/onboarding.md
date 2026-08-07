# Developer Onboarding

This checklist takes a fresh development checkout from clone to a tested local
server. For player and builder orientation, use
[Getting Started](GETTING_STARTED.md).

## Prerequisites

- [ ] Linux, a Linux-compatible Unix environment, or Ubuntu under WSL2
- [ ] GCC 13+ or Clang 18+ with the repository's GNU C23 feature support
- [ ] GNU Autotools; CMake 3.21+ is supported as an alternative
- [ ] MariaDB/MySQL server and development headers
- [ ] Development libraries for crypt, GD, curl, OpenSSL, pthread, and json-c

The automated deployment script installs supported Ubuntu/Debian dependencies.
See the [setup and build guide](guides/SETUP_AND_BUILD_GUIDE.md) for exact
packages and manual paths.

## Setup

1. Clone and enter the repository:

   ```bash
   git clone https://github.com/LuminariMUD/Luminari-Source.git
   cd Luminari-Source
   ```

2. Run the one-command development setup:

   ```bash
   ./scripts/deployment/deploy.sh --dev
   ```

   The script prepares missing local configuration, MariaDB, minimal world
   data, the Autotools build, and the installed executable. It does not make
   local credential files safe to commit; `lib/mysql_config` and `lib/.env`
   remain protected local files.

3. Verify the repository gate:

   ```bash
   make test
   make install
   ```

4. Start the server and verify readiness:

   ```bash
   ./bin/circle -d lib
   ./scripts/operations/healthcheck.sh
   ```

   Run the health check from a second terminal while the server is active.
   The default game and health ports are 4100 and 8182, respectively.

## Read Before Editing

- [Contributing rules](../CONTRIBUTING.md)
- [Development commands and repository map](development.md)
- [Architecture](ARCHITECTURE.md)
- [Testing guide](guides/TESTING_GUIDE.md)
- [Technical documentation index](TECHNICAL_DOCUMENTATION_MASTER_INDEX.md)

Local configuration headers and credential files are intentionally ignored.
Never overwrite an existing local configuration with its example template.
