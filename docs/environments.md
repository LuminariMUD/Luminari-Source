# LuminariMUD Environments

This document describes the different environments for running LuminariMUD and their configuration differences.

## Environment Overview

| Environment | URL/Port | Purpose |
|-------------|----------|---------|
| Development | localhost:4000 | Local development and testing |
| Staging | (internal):4000 | Pre-production testing |
| Production | luminarimud.com:4000 | Live game server |

## Environment Types

### Development

Local development environment for coding and testing.

**Characteristics:**
- Debug symbols enabled
- Verbose logging
- Fast iteration cycle
- Test data may be reset frequently
- Single-user or limited multi-user testing

**Configuration:**
```bash
# Build with debug symbols
./configure --enable-debug
make

# Or with CMake
cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build/
cmake --build build/
```

**Environment Variables:**
```bash
# Optional: Override default port
export LUMINARI_PORT=4000

# Optional: Enable verbose logging
export LUMINARI_DEBUG=1
```

### Staging

Pre-production environment for integration testing and QA.

**Characteristics:**
- Production-like configuration
- Periodic data resets
- Used for testing new features before production
- May use production data snapshots (sanitized)

**Configuration:**
```bash
# Build optimized but with some debug info
./configure
make

# Database: Use staging database credentials
# Copy staging-specific configs
```

### Production

Live game server for players.

**Characteristics:**
- Optimized build (-O2)
- Minimal logging (errors only)
- Persistent data
- High availability requirements
- Regular backups

**Configuration:**
```bash
# Build optimized
./configure --disable-debug
make

# Or with CMake
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build/
cmake --build build/
```

## Configuration Files

### Required Configuration (All Environments)

These files must be created from templates before building:

| File | Template | Purpose |
|------|----------|---------|
| `src/campaign.h` | `src/campaign.example.h` | World settings, campaign name |
| `src/mud_options.h` | `src/mud_options.example.h` | Feature toggles, server options |
| `src/vnums.h` | `src/vnums.example.h` | Virtual number assignments |

### Environment-Specific Settings

#### Development Settings

```c
/* In campaign.h */
#define CAMPAIGN_NAME "LuminariDev"
#define DEVMODE 1

/* In mud_options.h */
#define DEBUG_MEMORY 1
#define VERBOSE_LOGGING 1
```

#### Production Settings

```c
/* In campaign.h */
#define CAMPAIGN_NAME "LuminariMUD"
#define DEVMODE 0

/* In mud_options.h */
#define DEBUG_MEMORY 0
#define VERBOSE_LOGGING 0
```

## Database Configuration

### Development Database

```
Host: localhost
Database: luminari_dev
User: luminari_dev
Password: (local development password)
```

### Staging Database

```
Host: (staging-db-host)
Database: luminari_staging
User: luminari_staging
Password: (from secrets manager)
```

### Production Database

```
Host: (production-db-host)
Database: luminari_mudprod
User: luminari_mud
Password: (from secrets manager)
```

**Important:** Never commit database credentials. Use environment variables or secure secret storage.

## Port Configuration

| Environment | Default Port | Notes |
|-------------|--------------|-------|
| Development | 4000 | Can be changed via -p flag |
| Staging | 4000 | Behind staging firewall |
| Production | 4000 | Behind production firewall/load balancer |

## Logging Differences

### Development
- All log levels enabled
- Logs to stdout and syslog
- Debug messages included
- Performance profiling available

### Staging
- Info and above
- Logs to syslog
- Some debug messages for testing

### Production
- Warning and above
- Logs to syslog
- Error alerts configured
- Log rotation enabled

## Data Management

### Development
- May use minimal test data
- Frequent resets acceptable
- Can use `scripts/deploy.sh` for fresh setup

### Staging
- Uses production data snapshot (sanitized)
- Reset weekly or as needed
- Player passwords replaced with test passwords

### Production
- Live player data
- Continuous backups
- No automated resets
- Strict change management

## Deployment Process

### Development
```bash
# Quick rebuild
./cbuild.sh

# Or manual
make clean && make
```

### Staging
```bash
# Deploy script handles staging
./scripts/deploy.sh --staging
```

### Production
```bash
# Production deployment (requires approval)
./scripts/deploy.sh --prod
```

See [DEPLOYMENT_GUIDE.md](deployment/DEPLOYMENT_GUIDE.md) for full deployment procedures.

## Monitoring

### Development
- Manual observation
- Debug output in console
- Valgrind for memory checking

### Staging
- Basic monitoring
- Error log review
- Performance baseline testing

### Production
- Full monitoring stack
- Alerting for errors
- Performance metrics
- Player count tracking

## Troubleshooting

For environment-specific issues, see:
- [TROUBLESHOOTING_AND_MAINTENANCE.md](guides/TROUBLESHOOTING_AND_MAINTENANCE.md)
- [VESSEL_TROUBLESHOOTING.md](guides/VESSEL_TROUBLESHOOTING.md) (for vessel system)

## Security Considerations

1. **Never commit secrets** - Use `.env` files or secret managers
2. **Sanitize staging data** - Remove real player emails/passwords
3. **Firewall rules** - Staging/production behind firewalls
4. **Access control** - Limit SSH access to authorized personnel

---

*Last Updated: 2025-12-30*
