# Luminari MUD Frequently Asked Questions (FAQ)

## Table of Contents 

1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [Compilation Issues](#compilation-issues)
4. [Running the Server](#running-the-server)
5. [Administration](#administration)
6. [Development](#development)
7. [Performance and Optimization](#performance-and-optimization)
8. [Platform-Specific Issues](#platform-specific-issues)

---

## Introduction

### Q: I've never played a MUD before. What should I do?

**A:** Don't try to run your own MUD first! There are two types of MUD users: players and administrators. If you've never played a MUD, you should start by playing on existing MUDs to understand how they work. Running a MUD without understanding the player experience will leave you very confused.

**Recommendations:**
- Play several different MUDs to understand the genre
- Try both player and builder roles if possible
- Learn the common commands and gameplay mechanics
- Understand MUD culture and community management

### Q: I'm new to C programming. Can I still run a MUD?

**A:** While possible, a MUD is not a good learning project for C programming. MUDs have tens of thousands of lines of complex code that can be difficult even for experienced programmers.

**If you're determined to proceed:**
- Get a good C reference book
- Start with small modifications to existing code
- Avoid complex files like `comm.c` initially
- Practice with simple additions like new commands or items
- Use existing code as templates (copy, paste, modify)
- Be patient - learning takes time

### Q: What is Luminari MUD?

**A:** Luminari MUD is a derivative of tbaMUD, which itself comes from CircleMUD and ultimately DikuMUD. It's designed as a stable, extensible foundation for creating your own unique MUD rather than just another stock game.

**Key Features:**
- Highly optimized and stable codebase
- Online creation (OLC) system
- Advanced scripting capabilities
- Cross-platform compatibility
- Extensive documentation

---

## Getting Started

### Q: Where do I get the source code?

**A:** The Luminari MUD source code is available from the official repository. Make sure you download the latest stable version.

### Q: What platforms does Luminari MUD support?

**A:** Luminari MUD supports most modern platforms:
- **Linux** (all major distributions)
- **Unix variants** (Solaris, AIX, HP-UX, etc.)
- **macOS** (10.0 and above)
- **Windows** (with Cygwin or WSL)
- **BSD variants**
- Other POSIX-compliant systems

### Q: What are the system requirements?

**A:**
**Minimum (small/private MUD):**
- 20 MB disk space
- 32 MB RAM
- Basic Unix-like environment

**Recommended (production MUD):**
- 50+ MB disk space
- 50+ MB RAM
- Dedicated server or VPS
- Regular backup system

### Q: How many players can my MUD support?

**A:** This depends on your hardware and network connection. A typical server can handle 50-100 concurrent players comfortably. Monitor memory usage and response times to determine your limits. Memory is more important than CPU speed for MUDs.

---

## Compilation Issues

### Q: Why won't my MUD compile?

**A:** Common compilation issues include:

**Missing Dependencies:**
- Development libraries not installed
- Compiler not available
- Make utility missing

**Configuration Issues:**
- `./configure` not run properly
- Wrong compiler version
- Platform-specific problems

**Solutions:**
1. Ensure all development tools are installed
2. Run `./configure` before compiling
3. Check error messages for specific missing components
4. Consult platform-specific README files

### Q: I get syntax errors with Sun's 'cc' compiler. Why?

**A:** Sun's default `cc` compiler is not ANSI C compliant. Use `gcc` instead:
```bash
CC=gcc ./configure
make
```

### Q: I get errors about 'crypt' functions. What's wrong?

**A:** Some systems require linking with the crypt library. This is usually handled automatically by the configure script, but you may need to install development packages:
```bash
# On Debian/Ubuntu
sudo apt-get install libcrypt-dev

# On Red Hat/CentOS
sudo yum install glibc-devel
```

### Q: I get undefined symbols for socket functions. What's the problem?

**A:** Your system may need additional libraries for socket functions. The configure script should handle this, but you might need:
```bash
# Check if network libraries are available
sudo apt-get install libc6-dev
```

### Q: My compiler doesn't have 'strdup()'. What can I do?

**A:** Some older systems don't have `strdup()`. The MUD code should include a replacement function, but if not, you can define it yourself or use a more modern compiler.

### Q: What is a parse error and how do I fix it?

**A:** Parse errors usually indicate syntax problems in the code:
1. Check for missing semicolons or braces
2. Verify all includes are present
3. Make sure you haven't introduced syntax errors in modifications
4. Use a syntax-highlighting editor to spot issues

---

## Running the Server

### Q: I typed 'autorun' but my terminal froze. What happened?

**A:** The autorun script runs the MUD in the background. This is normal behavior. To see what's happening:
```bash
# Check if the MUD is running
ps aux | grep circle

# View the log files
tail -f log/syslog
```

### Q: The MUD says "Entering game loop" and stops. Is this normal?

**A:** Yes! This means the MUD is running successfully and waiting for connections. It's not frozen - it's ready for players to connect.

### Q: Why don't I get a login prompt when I connect?

**A:** Check these common issues:
1. **Wrong port:** Make sure you're connecting to the correct port
2. **Firewall:** Check if the port is blocked
3. **Binding issues:** The MUD might not be binding to the correct interface
4. **Client problems:** Try a different telnet client

### Q: I get "Error reading board: No such file or directory". What's wrong?

**A:** The MUD is looking for bulletin board files that don't exist. Create empty board files:
```bash
touch lib/etc/board.mortal
touch lib/etc/board.immortal
```

### Q: What is a SIGPIPE and how do I handle it?

**A:** SIGPIPE occurs when the MUD tries to write to a closed connection. This is normal and the MUD should handle it gracefully. If it's causing crashes, check your network code modifications.

### Q: My MUD crashed. What should I do?

**A:**
1. **Check for core dumps:** Look for `core` files in your MUD directory
2. **Review logs:** Check `log/syslog` and `log/syslog.CRASH`
3. **Restart:** Use the autorun script to restart automatically
4. **Debug:** Use `gdb` to analyze core dumps if available

### Q: How do I use gdb to debug crashes?

**A:**
```bash
# If you have a core dump
gdb bin/circle core

# Or attach to running process
gdb bin/circle <process_id>

# Common gdb commands
(gdb) bt          # Show backtrace
(gdb) info locals # Show local variables
(gdb) print var   # Print variable value
```

### Q: How do I keep the MUD running when I log off?

**A:** Use one of these methods:
```bash
# Using nohup
nohup ./autorun &

# Using screen
screen -S mud
./autorun
# Press Ctrl+A, then D to detach

# Using tmux
tmux new-session -s mud
./autorun
# Press Ctrl+B, then D to detach
```

---

## Administration

### Q: How do I create my first administrator character?

**A:**
1. Connect to your MUD and create a character normally
2. Log in as that character
3. Use the advance command: `advance <yourname> 34`
4. Set appropriate privileges as needed

### Q: How do I add new areas to my MUD?

**A:** You can add areas in several ways:
1. **Use OLC (Online Creation):** Build areas while the MUD is running
2. **Create world files manually:** Write .wld, .mob, .obj files
3. **Import existing areas:** Convert areas from other MUDs

Make sure to update the zone table and assign appropriate zone numbers.

### Q: How do I handle problem players?

**A:** Use administrative commands:
- `freeze <player>` - Temporarily disable account
- `mute <player>` - Prevent communication
- `ban <site>` - Ban by IP or hostname
- `purge <player>` - Permanently remove character

Always document incidents and maintain consistent policies.

### Q: What's the best way to backup player data?

**A:** Implement automated backups:
```bash
#!/bin/bash
# Daily backup script
DATE=$(date +%Y%m%d)
mkdir -p backups/$DATE
cp -r lib/plrfiles/ backups/$DATE/
cp -r lib/plrobjs/ backups/$DATE/
cp -r lib/etc/ backups/$DATE/
```

Run this script daily via cron and test restore procedures regularly.

### Q: How often should I restart the server?

**A:** Modern MUD servers can run for weeks or months without restart. Only restart when:
- Installing updates
- Memory leaks become problematic
- Configuration changes require it
- Performance degrades significantly

### Q: Can I modify the game while it's running?

**A:** Yes, partially:
- **World building:** Use OLC to modify areas, objects, and NPCs
- **Text files:** Edit help files, MOTD, etc. and use `reload`
- **Code changes:** Require compilation and restart

---

## Development

### Q: How do I add a new command?

**A:** Follow these steps:
1. Add the command to the command table in `interpreter.c`
2. Create the command function (usually in an appropriate `act.*.c` file)
3. Add the function declaration to the appropriate header file
4. Implement the command logic
5. Add help file entry

### Q: How do I add new classes or races?

**A:** This is a complex modification involving:
1. Updating constants and definitions
2. Modifying character creation code
3. Adding class/race-specific abilities
4. Updating save/load functions
5. Creating appropriate help files

Consider using existing patches or tutorials for guidance.

### Q: Can I run multiple MUDs on one server?

**A:** Yes, use different:
- Port numbers
- lib directories
- Binary names (optional)

Ensure adequate system resources for all instances.

### Q: How do I add new spells or skills?

**A:**
1. Define the spell/skill in appropriate header files
2. Add entries to skill/spell tables
3. Implement the spell/skill function
4. Add learning/practice mechanisms
5. Create help file entries
6. Test thoroughly

---

## Performance and Optimization

### Q: My MUD is running slowly. What should I check?

**A:** Monitor these factors:
- **Memory usage** (most common cause of slowdown)
- **CPU utilization**
- **Disk I/O** (especially player file operations)
- **Network connectivity**
- **Number of concurrent players**
- **Script efficiency** (DG Scripts, etc.)

### Q: How do I optimize performance?

**A:**
- **Memory management:** Regular restarts if leaks exist
- **Database optimization:** Efficient player file handling
- **Script optimization:** Review and optimize DG Scripts
- **Resource monitoring:** Limit resource-intensive operations
- **Code profiling:** Use profiling tools to identify bottlenecks

### Q: Why does my MUD use so much memory?

**A:** Common causes:
- Memory leaks in custom code
- Large numbers of objects in memory
- Inefficient string handling
- Accumulating log data
- Player file caching

Use memory profiling tools like `valgrind` to identify specific issues.

---

## Platform-Specific Issues

### Q: I'm having trouble compiling on Windows. What should I do?

**A:** Windows compilation options:
1. **Use WSL (Windows Subsystem for Linux)** - Recommended
2. **Use Cygwin** - Traditional approach
3. **Use MinGW** - Minimal GNU for Windows
4. **Use Visual Studio** - Requires project setup

See platform-specific README files for detailed instructions.

### Q: How do I set up automatic startup on Linux?

**A:** Create a systemd service:
```bash
# Create service file
sudo nano /etc/systemd/system/luminari.service

# Add service configuration
[Unit]
Description=Luminari MUD Server
After=network.target

[Service]
Type=forking
User=mud
WorkingDirectory=/path/to/mud
ExecStart=/path/to/mud/autorun
Restart=always

[Install]
WantedBy=multi-user.target

# Enable and start service
sudo systemctl enable luminari
sudo systemctl start luminari
```

### Q: I get "gethostbyaddr: connection refused" on Linux. Why?

**A:** This usually indicates DNS resolution issues. The MUD is trying to resolve hostnames for connecting players. This is often harmless but can be fixed by:
1. Configuring proper DNS settings
2. Disabling hostname resolution in the MUD configuration
3. Checking `/etc/resolv.conf`

---

## Additional Resources

### Documentation
- Administrator's Guide - Complete server administration
- Builder's Guide - World creation and OLC
- Developer's Guide - Code modification and programming
- Platform-specific README files

### Online Resources
- Official forums and communities
- MUD development websites
- Code repositories and patches
- Tutorial collections

### Getting Help
- Community forums
- Developer mailing lists
- IRC channels
- Discord servers

---

*This FAQ is based on the original tbaMUD FAQ, updated and expanded for Luminari MUD. For the most current information, check the official documentation and community resources.*

*Last updated: July 2025*
