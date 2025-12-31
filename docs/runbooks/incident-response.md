# LuminariMUD Incident Response Runbook

## Overview

This runbook provides procedures for responding to incidents affecting the LuminariMUD game server.

## Severity Levels

| Level | Description | Response Time | Example |
|-------|-------------|---------------|---------|
| P0 | Complete server outage | Immediate | Server crashed, all players disconnected |
| P1 | Major feature broken | < 1 hour | Combat system not working, login broken |
| P2 | Minor feature broken | < 4 hours | Single command broken, cosmetic issues |
| P3 | Minor/cosmetic | Next business day | Typo in message, minor display bug |

## Contact Information

| Role | Contact Method | Availability |
|------|----------------|--------------|
| Primary On-Call | Discord @oncall | 24/7 for P0/P1 |
| Core Team | Discord #dev-team | Business hours |
| Community Support | Discord #support | Best effort |

## Common Incidents

### Server Crash (P0)

**Symptoms:**
- All players disconnected
- Cannot connect to port 4000
- No response from server

**Diagnosis:**
```bash
# Check if server process is running
ps aux | grep circle

# Check port availability
netstat -tulpn | grep :4000

# Check recent logs
tail -100 lib/log/syslog
```

**Resolution:**
```bash
# 1. Check for core dump
ls -la lib/

# 2. Restart server
./bin/circle -d lib &

# Or use autorun for automatic restart
./autorun &

# 3. Verify server is running
telnet localhost 4000
```

**Post-Incident:**
- Review syslog for crash cause
- Check core dump with gdb if available
- Document incident in incident log
- Create GitHub issue if bug found

---

### Database Connection Lost (P1)

**Symptoms:**
- Players cannot save
- New logins fail
- Database-related errors in syslog

**Diagnosis:**
```bash
# Check MariaDB/MySQL service
sudo systemctl status mariadb
# or
sudo systemctl status mysql

# Check connection manually
mysql -u luminari_mud -p luminari_mudprod -e "SELECT 1;"

# Check recent database errors
tail -50 /var/log/mysql/error.log
```

**Resolution:**
```bash
# 1. Restart database service
sudo systemctl restart mariadb

# 2. If needed, restart MUD server to reconnect
# (send shutdown command in-game first if possible)

# 3. Verify connection restored
# Check syslog for successful reconnection
```

**Post-Incident:**
- Review database logs for root cause
- Check disk space if corruption suspected
- Verify all player data saved correctly

---

### Memory Leak (P1/P2)

**Symptoms:**
- Server becomes slow over time
- High memory usage in `top`
- Eventually crashes

**Diagnosis:**
```bash
# Check current memory usage
ps aux | grep circle

# Monitor over time
watch -n 60 'ps aux | grep circle'

# If possible, run with Valgrind
valgrind --leak-check=full ./bin/circle -d lib
```

**Resolution:**
```bash
# 1. Schedule server restart during low-activity period
# 2. Notify players of planned restart
# 3. Execute controlled shutdown in-game:
#    shutdown 5 Scheduled restart for maintenance

# 4. Restart server
./bin/circle -d lib &
```

**Post-Incident:**
- Identify leak source using Valgrind
- Create GitHub issue with Valgrind output
- Prioritize fix based on severity

---

### High CPU Usage (P2)

**Symptoms:**
- Server lag reported by players
- High CPU in `top`
- Slow command response

**Diagnosis:**
```bash
# Check CPU usage
top -p $(pgrep circle)

# Check for infinite loops in logs
tail -f lib/log/syslog | grep -i "loop\|hang\|stuck"

# Check connected player count
# (high player count = normal high CPU)
```

**Resolution:**
```bash
# 1. If caused by specific player/room, investigate
# 2. If general overload, consider:
#    - Reducing max connections temporarily
#    - Disabling resource-intensive features

# 3. If runaway process, controlled restart:
shutdown 2 Emergency maintenance restart
```

---

### Port Already in Use (P2)

**Symptoms:**
- Server won't start
- "Address already in use" error

**Diagnosis:**
```bash
# Find what's using the port
netstat -tulpn | grep :4000
lsof -i :4000
```

**Resolution:**
```bash
# 1. If old server process, kill it
kill -9 $(lsof -t -i:4000)

# 2. If another service, change port
./bin/circle -p 4001 -d lib

# 3. Restart server
./bin/circle -d lib &
```

---

### Corrupted World File (P2)

**Symptoms:**
- Server crashes on boot
- Specific zone fails to load
- "Error loading zone" in syslog

**Diagnosis:**
```bash
# Check recent syslog
grep -i "error\|corrupt\|fail" lib/log/syslog | tail -50

# Identify affected zone file
# Look for last successful zone load
```

**Resolution:**
```bash
# 1. Restore from backup
cp lib/world/backup/<zonefile>.wld lib/world/wld/

# 2. If no backup, disable zone temporarily
mv lib/world/wld/<problem>.wld lib/world/wld/<problem>.wld.disabled

# 3. Restart server
./bin/circle -d lib &
```

**Post-Incident:**
- Investigate cause of corruption
- Restore zone from backup or rebuild
- Re-enable zone after fix

---

### Vessel System Issue (P2/P3)

**Symptoms:**
- Vessel commands not working
- Vessels stuck at coordinates
- Interior room issues

**Diagnosis:**
```bash
# Check vessel-related errors
grep -i "vessel\|ship\|greyhawk" lib/log/syslog | tail -50

# Verify database tables
mysql -u luminari_mud -p luminari_mudprod -e "SELECT COUNT(*) FROM ship_interiors;"
```

**Resolution:**
See [VESSEL_TROUBLESHOOTING.md](../guides/VESSEL_TROUBLESHOOTING.md) for detailed procedures.

---

## Escalation Procedures

### When to Escalate

| Situation | Escalate To |
|-----------|-------------|
| P0 not resolved in 15 minutes | Core Team Lead |
| P1 not resolved in 1 hour | Core Team |
| Security incident | Core Team + Server Admin |
| Data loss confirmed | Core Team + Database Lead |

### Escalation Process

1. Document current status and actions taken
2. Contact appropriate person via Discord DM
3. Provide incident summary:
   - When it started
   - What symptoms observed
   - What actions attempted
   - Current status

## Post-Incident Review

After any P0 or P1 incident:

1. **Document Timeline**
   - When detected
   - When responded
   - When resolved
   - Total downtime

2. **Root Cause Analysis**
   - What caused the incident
   - Why it wasn't prevented
   - Contributing factors

3. **Action Items**
   - Preventive measures
   - Monitoring improvements
   - Documentation updates

4. **Create GitHub Issue**
   - Tag as `incident-review`
   - Link to any related bugs
   - Track action items

## Maintenance Windows

Regular maintenance windows for non-emergency work:

| Day | Time (UTC) | Duration | Activities |
|-----|------------|----------|------------|
| Sunday | 06:00 | 2 hours | Database maintenance, backups |
| As needed | (announced) | Varies | Deployments, upgrades |

### Scheduled Maintenance Procedure

1. Announce 24 hours in advance on Discord
2. Remind 1 hour before
3. Final warning 10 minutes before
4. Execute maintenance
5. Verify server healthy
6. Announce completion

---

*Last Updated: 2025-12-30*
*Review Frequency: Quarterly*
