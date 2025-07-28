# Luminari MUD Installation Guide

# Assembled by Zusuk
# Last Updated: July 27, 2025

# Note @LEGACY_README.md as well

## Table of Contents

1. [Overview](#overview)
2. [System Requirements](#system-requirements)
3. [Quick Start Guide](#quick-start-guide)
4. [Unix/Linux Installation](#unixlinux-installation)
5. [Windows Installation](#windows-installation)
6. [macOS Installation](#macos-installation)
7. [Platform-Specific Instructions](#platform-specific-instructions)
8. [Build System Configuration](#build-system-configuration)
9. [Troubleshooting](#troubleshooting)
10. [Post-Installation Setup](#post-installation-setup)

---

## Overview

Luminari MUD is designed to compile and run on a wide variety of platforms. This guide provides comprehensive installation instructions for all supported operating systems and compilers.

### Supported Platforms

**Primary Platforms:**
- **Linux** (all major distributions)
- **Unix variants** (Solaris, AIX, HP-UX, etc.)
- **Windows** (with multiple compiler options)
- **macOS** (10.0 and above)
- **BSD variants** (FreeBSD, OpenBSD, NetBSD)

**Compiler Support:**
- **GCC** (GNU Compiler Collection) - Recommended
- **Clang** (LLVM Compiler)
- **Microsoft Visual C++** (various versions)
- **Borland C++**
- **Watcom C++**

### Installation Philosophy

Luminari MUD uses the GNU autotools build system, which automatically detects your system's capabilities and configures the build process accordingly. This means that in most cases, installation is as simple as:

```bash
./configure
make
```

However, some platforms may require additional steps or specific configurations, which are detailed in this guide.

---

## System Requirements

### Minimum Requirements

**Hardware:**
- **CPU:** Any modern processor (x86, x64, ARM)
- **RAM:** 32 MB (small/private MUD)
- **Disk Space:** 20 MB for source and binaries
- **Network:** TCP/IP networking capability

**Software:**
- **C Compiler:** ANSI C compatible compiler
- **Make Utility:** GNU Make or compatible
- **Standard Libraries:** POSIX-compliant system libraries
- **Network Libraries:** Socket support (usually built-in)

### Recommended Requirements

**For Production Use:**
- **RAM:** 64+ MB
- **Disk Space:** 50+ MB (including logs and player data)
- **Backup System:** Regular automated backups
- **Monitoring:** System monitoring tools

**Development Environment:**
- **Debugger:** GDB or compatible debugger
- **Version Control:** Git for source management
- **Text Editor:** Editor with C syntax highlighting
- **Documentation Tools:** For maintaining documentation

---

## Quick Start Guide

For experienced users who want to get up and running quickly:

### Linux/Unix Quick Start
```bash
# Download and extract source
tar -xzf luminari-source.tar.gz
cd Luminari-Source

# Configure and build
./configure
cd src
make

# Run the MUD
cd ../bin
./autorun &
```

### Windows Quick Start (Cygwin)
```bash
# Install Cygwin with development tools
# Download source and extract
tar -xzf luminari-source.tar.gz
cd Luminari-Source

# Configure and build
./configure
cd src
make

# Run the MUD
cd ../bin
./autorun &
```

### macOS Quick Start
```bash
# Install Xcode command line tools
xcode-select --install

# Download and extract source
tar -xzf luminari-source.tar.gz
cd Luminari-Source

# Configure and build
./configure
cd src
make

# Run the MUD
cd ../bin
./autorun &
```

---

## Unix/Linux Installation

### Prerequisites

**Package Installation:**

**Debian/Ubuntu:**
```bash
sudo apt-get update
sudo apt-get install build-essential autoconf automake
sudo apt-get install git wget curl  # Optional but recommended
```

**Red Hat/CentOS/Fedora:**
```bash
sudo yum groupinstall "Development Tools"
sudo yum install autoconf automake
# Or for newer versions:
sudo dnf groupinstall "Development Tools"
sudo dnf install autoconf automake
```

**SUSE/openSUSE:**
```bash
sudo zypper install -t pattern devel_basis
sudo zypper install autoconf automake
```

### Step-by-Step Installation

**Step 1: Download Source Code**
```bash
# If downloading from repository
git clone <repository-url> Luminari-Source
cd Luminari-Source

# If downloading archive
wget <download-url>/luminari-source.tar.gz
tar -xzf luminari-source.tar.gz
cd Luminari-Source
```

**Step 2: Configure the Build**
```bash
./configure
```

The configure script will:
- Detect your system type and capabilities
- Check for required libraries and functions
- Create appropriate Makefiles and configuration headers
- Display a summary of detected features

**Step 3: Compile the Source**
```bash
cd src
make
```

This will compile:
- The main MUD server (`circle`)
- Utility programs (if requested with `make utils`)

**Step 4: Verify Installation**
```bash
# Check that the binary was created
ls -la ../bin/circle

# Test basic functionality
../bin/circle -c  # Syntax check mode
```

**Step 5: Initial Setup**
```bash
cd ..
# Copy default configuration files if needed
# Set up initial world files
# Configure autorun script
```

### Platform-Specific Notes

**Linux Distributions:**

**Ubuntu/Debian:**
- Usually works out of the box with build-essential
- May need `libcrypt-dev` for some versions
- Consider using `apt-get build-dep` for dependencies

**CentOS/RHEL:**
- Older versions may need EPEL repository
- Some versions require `glibc-devel` package
- Check SELinux settings if having permission issues

**Arch Linux:**
```bash
sudo pacman -S base-devel autoconf automake
```

**Gentoo:**
```bash
emerge --ask sys-devel/autoconf sys-devel/automake
```

### Unix Variants

**Solaris:**
- Use GNU tools when available (`/usr/sfw/bin` or `/opt/csw/bin`)
- May need to set `CC=gcc` before configure
- Check for required libraries in `/usr/lib` or `/opt/lib`

**AIX:**
- Use GCC if available, or IBM XL C compiler
- May need to set specific compiler flags
- Check for required networking libraries

**HP-UX:**
- Use GCC or HP ANSI C compiler
- May need specific networking libraries
- Check for POSIX compliance issues

---

## Windows Installation

Windows users have several compiler options available. Each has different installation procedures and requirements.

### Option 1: Cygwin (Recommended for Beginners)

Cygwin provides a Unix-like environment on Windows and is the easiest way to compile Luminari MUD.

**Installation Steps:**

**Step 1: Download and Install Cygwin**
1. Visit https://www.cygwin.com/
2. Download the appropriate installer (32-bit or 64-bit)
3. Run the installer and select packages:
   - **Base:** Default packages
   - **Devel:** gcc-core, gcc-g++, make, autoconf, automake
   - **Net:** wget, curl (optional)
   - **Utils:** tar, gzip, unzip

**Step 2: Compile Luminari MUD**
```bash
# Open Cygwin terminal
cd /cygdrive/c/your-source-directory
tar -xzf luminari-source.tar.gz
cd Luminari-Source

# Configure and build
./configure
cd src
make

# Run the MUD
cd ../bin
./autorun &
```

**Cygwin Notes:**
- Paths use Unix-style forward slashes
- Windows drives are accessible as `/cygdrive/c/`, `/cygdrive/d/`, etc.
- The MUD will run in the Cygwin environment
- Performance may be slightly lower than native Windows compilation

### Option 2: Windows Subsystem for Linux (WSL)

WSL provides a full Linux environment on Windows 10/11.

**Installation Steps:**

**Step 1: Enable WSL**
```powershell
# Run as Administrator
wsl --install
# Or for older Windows versions:
dism.exe /online /enable-feature /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart
```

**Step 2: Install Linux Distribution**
```powershell
# Install Ubuntu (recommended)
wsl --install -d Ubuntu
```

**Step 3: Compile Luminari MUD**
```bash
# In WSL terminal
sudo apt-get update
sudo apt-get install build-essential autoconf automake git

# Download and compile
git clone <repository-url> Luminari-Source
cd Luminari-Source
./configure
cd src
make

# Run the MUD
cd ../bin
./autorun &
```

### Option 3: Microsoft Visual Studio

Visual Studio provides native Windows compilation with excellent debugging tools.

**Supported Versions:**
- Visual Studio 2019 (recommended)
- Visual Studio 2017
- Visual Studio 2015
- Visual Studio Community (free version)

**Installation Steps:**

**Step 1: Install Visual Studio**
1. Download Visual Studio from Microsoft
2. Install with C++ development tools
3. Ensure Windows SDK is included

**Step 2: Prepare Project Files**
```cmd
# Open Developer Command Prompt
cd C:\path\to\Luminari-Source
# Use provided project files or create new ones
```

**Step 3: Build Project**
1. Open the solution file in Visual Studio
2. Select Release or Debug configuration
3. Build the solution (Ctrl+Shift+B)
4. Check for any compilation errors

**Visual Studio Notes:**
- Project files may need updating for newer versions
- Check for Windows-specific code paths
- Use Visual Studio debugger for troubleshooting
- Consider using vcpkg for dependency management

### Option 4: MinGW-w64

MinGW provides GCC compilation on Windows without requiring Cygwin.

**Installation Steps:**

**Step 1: Install MinGW-w64**
1. Download from https://www.mingw-w64.org/
2. Install to a directory without spaces (e.g., C:\mingw64)
3. Add bin directory to PATH environment variable

**Step 2: Install MSYS2 (Optional but Recommended)**
1. Download MSYS2 from https://www.msys2.org/
2. Install and update package database
3. Install development tools:
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make
```

**Step 3: Compile Luminari MUD**
```cmd
# In Command Prompt or MSYS2 terminal
cd C:\path\to\Luminari-Source
# May need to modify Makefile for Windows paths
mingw32-make
```

---

## macOS Installation

macOS provides excellent support for Unix-style development with some platform-specific considerations.

### Prerequisites

**Install Xcode Command Line Tools:**
```bash
xcode-select --install
```

This provides:
- GCC/Clang compiler
- Make utility
- Standard development libraries
- Git version control

**Alternative: Install Xcode**
- Download from Mac App Store
- Provides full IDE and additional tools
- Larger download but more comprehensive

### Installation Steps

**Step 1: Prepare Development Environment**
```bash
# Verify tools are installed
gcc --version
make --version
git --version
```

**Step 2: Download Source Code**
```bash
# Using Git
git clone <repository-url> Luminari-Source
cd Luminari-Source

# Or download archive
curl -O <download-url>/luminari-source.tar.gz
tar -xzf luminari-source.tar.gz
cd Luminari-Source
```

**Step 3: Configure and Build**
```bash
./configure
cd src
make
```

**Step 4: Test Installation**
```bash
cd ../bin
./circle -c  # Syntax check
./autorun &  # Run in background
```

### macOS-Specific Notes

**Homebrew Integration:**
If using Homebrew package manager:
```bash
# Install additional tools if needed
brew install autoconf automake
brew install wget curl  # Optional utilities
```

**Path Considerations:**
- Default installation paths work well
- Consider using `/usr/local/` for custom installations
- Avoid spaces in directory names

**Performance Notes:**
- macOS provides excellent performance for MUD servers
- Consider using Activity Monitor to track resource usage
- Built-in firewall may need configuration for external access

---

## Platform-Specific Instructions

### BSD Variants

**FreeBSD:**
```bash
# Install development tools
pkg install autotools gcc
# Or from ports
cd /usr/ports/devel/autotools && make install clean

# Compile normally
./configure
cd src
make
```

**OpenBSD:**
```bash
# Install packages
pkg_add autoconf automake gcc
# Compile with specific flags if needed
CC=gcc ./configure
cd src
make
```

**NetBSD:**
```bash
# Use pkgsrc
cd /usr/pkgsrc/devel/autoconf && make install
cd /usr/pkgsrc/devel/automake && make install

# Compile normally
./configure
cd src
make
```

### Legacy Unix Systems

**IRIX (SGI):**
- Use GCC if available, or SGI MIPSpro compiler
- May need specific networking libraries
- Check for 64-bit vs 32-bit compilation issues

**Digital Unix/Tru64:**
- Use GCC or DEC C compiler
- May need specific compiler flags for optimization
- Check for Alpha-specific issues

**SCO Unix:**
- Limited support due to licensing restrictions
- May require specific compiler and library configurations
- Consider using GCC if available

### Embedded Systems

**Raspberry Pi:**
```bash
# Standard Debian/Ubuntu instructions work
sudo apt-get install build-essential autoconf automake
# Compile normally but expect slower performance
```

**ARM-based Systems:**
- Most ARM Linux distributions work normally
- May need cross-compilation for some embedded targets
- Consider memory limitations for smaller systems

---

## Build System Configuration

### Configure Script Options

The configure script accepts various options to customize the build:

**Common Options:**
```bash
./configure --help                    # Show all options
./configure --prefix=/usr/local       # Set installation prefix
./configure --enable-debug            # Enable debugging symbols
./configure --disable-shared          # Disable shared libraries
./configure CC=gcc                    # Specify compiler
./configure CFLAGS="-O2 -g"          # Set compiler flags
```

**Advanced Options:**
```bash
# Cross-compilation
./configure --host=arm-linux-gnueabihf

# Custom library paths
./configure --with-libs=/usr/local/lib

# Disable specific features
./configure --disable-feature-name
```

### Makefile Customization

**Common Makefile Variables:**
```makefile
CC = gcc                    # Compiler
CFLAGS = -O2 -g -Wall      # Compiler flags
LDFLAGS = -L/usr/local/lib # Linker flags
LIBS = -lm -lcrypt         # Libraries to link
```

**Platform-Specific Makefiles:**
- `Makefile.unix` - Generic Unix systems
- `Makefile.linux` - Linux-specific optimizations
- `Makefile.bsd` - BSD variants
- `Makefile.win32` - Windows with MinGW

### Configuration Headers

**conf.h Customization:**
The configure script creates `conf.h` with system-specific definitions:

```c
#define HAVE_CRYPT_H 1        // System has crypt.h
#define HAVE_SYS_SOCKET_H 1   // System has socket support
#define SIZEOF_LONG 8         // Size of long integer
```

**Manual Configuration:**
For systems without autoconf support, manually edit `conf.h`:
1. Copy `conf.h.in` to `conf.h`
2. Define appropriate macros for your system
3. Test compilation and adjust as needed
---

## Troubleshooting

### Common Compilation Issues

**"configure: command not found"**
- Solution: Install autoconf and automake packages
- Alternative: Use pre-generated Makefile if available

**"gcc: command not found"**
- Solution: Install development tools for your platform
- Check that compiler is in PATH environment variable

**"Permission denied" when running configure**
- Solution: Make configure script executable: `chmod +x configure`
- Alternative: Run with shell: `sh ./configure`

**Missing header files (e.g., "sys/socket.h not found")**
- Solution: Install development packages for your distribution
- Check for platform-specific header locations

**Linker errors about missing libraries**
- Solution: Install required library development packages
- Check library paths and names for your platform

**"make: *** No targets specified and no makefile found"**
- Solution: Run configure first to generate Makefile
- Check that you're in the correct directory

### Platform-Specific Issues

**Linux Issues:**

**SELinux Problems:**
```bash
# Check SELinux status
sestatus
# Temporarily disable if needed
sudo setenforce 0
```

**Library Path Issues:**
```bash
# Add library paths
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
# Or add to /etc/ld.so.conf and run ldconfig
```

**Windows Issues:**

**Cygwin Path Problems:**
- Use Unix-style paths in Cygwin terminal
- Convert Windows paths: `cygpath -u "C:\path"`

**Visual Studio Compilation Errors:**
- Check Windows SDK version compatibility
- Verify all required components are installed
- Check for Unicode vs ANSI string issues

**macOS Issues:**

**Xcode Command Line Tools Missing:**
```bash
# Reinstall command line tools
sudo xcode-select --reset
xcode-select --install
```

**Library Linking Issues:**
```bash
# Check library paths
otool -L bin/circle
# Fix library paths if needed
install_name_tool -change old_path new_path bin/circle
```

### Runtime Issues

**"Address already in use" Error:**
- Another process is using the port
- Wait for previous instance to fully shut down
- Use different port number
- Kill existing process: `killall circle`

**"Permission denied" for Port Binding:**
- Ports below 1024 require root privileges
- Use port 4000 or higher for non-root operation
- Or run with sudo (not recommended for production)

**Segmentation Fault on Startup:**
- Check for corrupted data files
- Run with debugger: `gdb bin/circle`
- Check system logs for additional information

**Players Cannot Connect:**
- Check firewall settings
- Verify port is open: `netstat -an | grep :4000`
- Test local connection first: `telnet localhost 4000`

---

## Post-Installation Setup

### Initial Configuration

**Step 1: Set Up Directory Structure**
```bash
# Ensure all required directories exist
mkdir -p lib/etc lib/plrfiles lib/plrobjs lib/house
mkdir -p log

# Set appropriate permissions
chmod 755 lib log
chmod 644 lib/etc/*
```

**Step 2: Configure Basic Settings**
Edit `lib/etc/config` to set:
- Server name and description
- Administrator email
- Port number
- Player limits
- Basic game settings

**Step 3: Create Initial Administrator**
```bash
# Start the MUD
cd bin
./circle &

# Connect and create character
telnet localhost 4000
# Create character normally, then advance to implementor level
```

### Security Configuration

**File Permissions:**
```bash
# Secure configuration files
chmod 600 lib/etc/config
chmod 600 lib/etc/badsites

# Secure player files
chmod 700 lib/plrfiles lib/plrobjs

# Secure log files
chmod 640 log/*
```

**Network Security:**
- Configure firewall to allow only necessary ports
- Consider using fail2ban for brute force protection
- Monitor connection logs regularly
- Use strong passwords for administrator accounts

### Performance Tuning

**System Limits:**
```bash
# Check current limits
ulimit -a

# Increase file descriptor limit if needed
ulimit -n 4096

# For permanent changes, edit /etc/security/limits.conf
```

**Memory Management:**
- Monitor memory usage with `top` or `htop`
- Set up log rotation to prevent disk space issues
- Consider automatic restarts if memory leaks occur

**Network Optimization:**
- Tune TCP buffer sizes for your expected player load
- Monitor network connections with `netstat`
- Consider using connection pooling for high loads

### Backup and Maintenance

**Automated Backups:**
```bash
#!/bin/bash
# backup.sh - Daily backup script
DATE=$(date +%Y%m%d)
BACKUP_DIR="/backups/luminari/$DATE"

mkdir -p "$BACKUP_DIR"
cp -r lib/plrfiles "$BACKUP_DIR/"
cp -r lib/plrobjs "$BACKUP_DIR/"
cp -r lib/etc "$BACKUP_DIR/"

# Compress old backups
find /backups/luminari -name "*.tar.gz" -mtime +30 -delete
tar -czf "$BACKUP_DIR.tar.gz" "$BACKUP_DIR"
rm -rf "$BACKUP_DIR"
```

**Log Rotation:**
```bash
# Add to /etc/logrotate.d/luminari
/path/to/mud/log/* {
    daily
    missingok
    rotate 30
    compress
    notifempty
    create 644 muduser mudgroup
}
```

**Monitoring Scripts:**
```bash
#!/bin/bash
# monitor.sh - Check if MUD is running
if ! pgrep -f "circle" > /dev/null; then
    echo "MUD is down, restarting..."
    cd /path/to/mud/bin
    ./autorun &
fi
```

### Integration with System Services

**Systemd Service (Linux):**
```ini
# /etc/systemd/system/luminari.service
[Unit]
Description=Luminari MUD Server
After=network.target

[Service]
Type=forking
User=muduser
Group=mudgroup
WorkingDirectory=/path/to/mud
ExecStart=/path/to/mud/bin/autorun
ExecReload=/bin/kill -HUP $MAINPID
KillMode=process
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

**Enable and start service:**
```bash
sudo systemctl enable luminari
sudo systemctl start luminari
sudo systemctl status luminari
```

### Development Environment Setup

**Version Control:**
```bash
# Initialize Git repository
git init
git add .
git commit -m "Initial Luminari MUD setup"

# Set up remote repository
git remote add origin <repository-url>
git push -u origin master
```

**Development Tools:**
```bash
# Install additional development tools
# For debugging
sudo apt-get install gdb valgrind

# For profiling
sudo apt-get install gprof

# For documentation
sudo apt-get install doxygen
```

---

## Conclusion

Congratulations! You have successfully installed Luminari MUD on your system. This installation guide has covered the major platforms and common issues you might encounter.

### Next Steps

1. **Read the Administrator's Guide** for server management
2. **Review the Builder's Manual** for world creation
3. **Study the Developer's Guide** for code modification
4. **Join the community** for support and collaboration

### Getting Help

If you encounter issues not covered in this guide:

1. **Check the FAQ** in the Administrator's Guide
2. **Search community forums** for similar issues
3. **Review system logs** for error messages
4. **Ask for help** in community channels

### Contributing Back

If you successfully port Luminari MUD to a new platform or solve installation issues:

1. **Document your solution** clearly
2. **Create patches** for any code changes needed
3. **Submit your contributions** to the community
4. **Help others** who encounter similar issues

Remember that the MUD community thrives on collaboration and knowledge sharing. Your contributions help make Luminari MUD accessible to more developers and administrators.

---

## Additional Resources

### Official Documentation
- **Administrator's Guide:** Complete server management
- **Builder's Manual:** World creation and building
- **Developer's Guide:** Code modification and programming
- **FAQ Collection:** Common questions and solutions

### Community Resources
- **Forums:** Discussion and support
- **Wiki:** Community-maintained documentation
- **Code Repositories:** Patches and enhancements
- **Mailing Lists:** Developer communication

### Technical References
- **GNU Autotools:** Build system documentation
- **GCC Manual:** Compiler documentation
- **Platform Documentation:** OS-specific development guides
- **Network Programming:** Socket and TCP/IP references

---

*This installation guide consolidates information from all platform-specific README files and installation documentation, updated and expanded for modern systems and development practices.*

*Last updated: July 27, 2025*