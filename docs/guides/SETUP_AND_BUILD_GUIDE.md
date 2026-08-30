# LuminariMUD Setup and Build Guide

## Supported Setup

LuminariMUD builds on Linux and Linux-compatible environments such as Ubuntu
under WSL2. The server requires MariaDB/MySQL and a compiler with the
repository's GNU C23 feature support.

The preferred fresh-install path is:

```bash
./scripts/deployment/deploy.sh
```

That command installs supported dependencies, creates missing local
configuration from examples, provisions MariaDB, initializes minimal world
data, configures Autotools, builds, and installs `bin/luminari`. Inspect exact
flags with `./scripts/deployment/deploy.sh --help`.

## Ubuntu, Debian, and WSL2 Dependencies

```bash
sudo apt-get update
sudo apt-get install -y build-essential git make autoconf automake libtool \
  cmake pkg-config mariadb-server libmariadb-dev libcrypt-dev libgd-dev \
  libcurl4-openssl-dev libssl-dev libjson-c-dev zlib1g-dev mariadb-client \
  pandoc gdb valgrind
```

## Existing Configured Checkout

Autotools is preferred for incremental development:

```bash
make clean
make -j"$(nproc)"
make test
make install
```

If the generated build files are absent:

```bash
autoreconf -fvi
./configure
make -j"$(nproc)"
make test
make install
```

`make test` may leave a root-level test build of `luminari`; the required
`make install` step activates the versioned binary at `bin/luminari` and removes
that root artifact.

## Fresh Manual Configuration

Only when the real local files do not exist, copy the tracked examples:

```bash
test -e src/campaign.h || cp src/campaign.example.h src/campaign.h
test -e src/mud_options.h || cp src/mud_options.example.h src/mud_options.h
test -e src/vnums.h || cp src/vnums.example.h src/vnums.h
test -e lib/mysql_config || install -m 600 lib/mysql_config_example lib/mysql_config
test -e lib/.env || install -m 600 lib/.env_example lib/.env
```

Edit local configuration without committing it. Never overwrite an existing
`src/campaign.h`, `src/mud_options.h`, `src/vnums.h`, `lib/mysql_config`, or
`lib/.env`. Database initialization details are in the
[deployment guide](../deployment/DEPLOYMENT_GUIDE.md) and
[database initialization guide](DATABASE_INITIALIZATION_GUIDE.md).

World and text data must exist under `lib/`. Use the deployment script for a
fresh minimal world rather than assembling the required indexes manually.

## CMake

CMake is the supported secondary build. Tests are disabled by default and must
be enabled explicitly for validation:

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
cmake --install build
```

## Run and Verify

```bash
./bin/luminari -d lib
```

The checked-in runtime configuration defaults to the reserved local game port
4101. While the server runs, verify the loopback health listener from another
terminal:

```bash
./scripts/operations/healthcheck.sh
```

See [development.md](../development.md) for daily commands,
[TESTING_GUIDE.md](TESTING_GUIDE.md) for all test surfaces, and
[incident-response.md](../runbooks/incident-response.md) for operational
diagnosis.
