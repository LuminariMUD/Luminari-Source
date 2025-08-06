# LuminariMUD CMake Build Guide

## Overview

This guide covers building LuminariMUD using CMake as an alternative to the traditional Makefile approach. CMake provides cross-platform build configuration and better IDE integration.

**IMPORTANT**: LuminariMUD uses ANSI C90/C89 standard, NOT C99. The CMake configuration enforces this requirement.

## Prerequisites

- CMake 3.12 or higher
- GCC compiler with C90 support
- MySQL/MariaDB development libraries
- GD graphics library (optional)

## Quick Start

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build
make -j4

# Install to ../bin
make install
```

## Configuration Options

### Basic Options

```bash
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build with utilities
cmake -DBUILD_UTILS=ON ..

# Build with unit tests
cmake -DBUILD_TESTS=ON ..
```

### Feature Toggles

```bash
# Enable memory debugging
cmake -DMEMORY_DEBUG=ON ..

# Enable dmalloc support
cmake -DDMALLOC=ON ..

# Enable developer warnings
cmake -DDEVELOPER_MODE=ON ..

# Enable static analysis with clang-tidy
cmake -DSTATIC_ANALYSIS=ON ..
```

### Library Paths

If CMake can't find your libraries automatically:

```bash
# Specify MySQL paths
cmake -DMYSQL_INCLUDE_DIR=/usr/include/mysql \
      -DMYSQL_LIBRARY=/usr/lib/libmysqlclient.so ..

# Specify GD library
cmake -DGD_LIBRARY=/usr/lib/libgd.so ..
```

## Build Targets

```bash
# Build main server
make circle

# Build specific utility
make autowiz

# Build all utilities
cmake -DBUILD_UTILS=ON .. && make

# Build unit tests
cmake -DBUILD_TESTS=ON .. && make cutest

# Clean build
make clean
```

## IDE Integration

### Visual Studio Code

Create `.vscode/settings.json`:
```json
{
    "cmake.sourceDirectory": "${workspaceFolder}",
    "cmake.buildDirectory": "${workspaceFolder}/build",
    "cmake.configureSettings": {
        "CMAKE_BUILD_TYPE": "Debug",
        "BUILD_UTILS": "ON"
    }
}
```

### CLion

1. Open project root containing CMakeLists.txt
2. CLion will automatically detect CMake project
3. Configure build profiles in Settings → Build → CMake

### Eclipse CDT

```bash
# Generate Eclipse project files
mkdir build-eclipse
cd build-eclipse
cmake -G"Eclipse CDT4 - Unix Makefiles" ..
```

## Troubleshooting

### C99 Features Error

If you get errors about C99 features:
```bash
# Ensure C90 standard is enforced
cmake -DCMAKE_C_STANDARD=90 -DCMAKE_C_STANDARD_REQUIRED=ON ..
```

### Missing MySQL

```bash
# Ubuntu/Debian
sudo apt-get install libmysqlclient-dev

# CentOS/RHEL
sudo yum install mysql-devel

# Specify paths manually
cmake -DMYSQL_INCLUDE_DIR=/path/to/mysql/include \
      -DMYSQL_LIBRARY=/path/to/libmysqlclient.so ..
```

### Configuration Headers Missing

```
CMake Error: campaign.h not found!
```

Solution:
```bash
cp campaign.example.h campaign.h
cp mud_options.example.h mud_options.h
cp vnums.example.h vnums.h
# Edit files as needed, then re-run cmake
```

## Comparison with Autotools Build

| Feature | Autotools | CMake |
|---------|-----------|--------|
| Configure | `./configure` | `cmake ..` |
| Build | `make` | `make` |
| Clean | `make clean` | `make clean` |
| Utilities | `make utils` | `cmake -DBUILD_UTILS=ON ..` |
| IDE Support | Limited | Excellent |
| Cross-platform | Unix-like only | Windows, Mac, Linux |

## Advanced Usage

### Custom Compiler Flags

```bash
# Add custom flags
cmake -DCMAKE_C_FLAGS="-march=native -mtune=native" ..

# Debug with sanitizers
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_C_FLAGS="-fsanitize=address -fsanitize=undefined" ..
```

### Export Compile Commands

For tools like clang-tidy, clangd:
```bash
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
```

### Parallel Builds

```bash
# Use all CPU cores
make -j$(nproc)

# Or specify number
make -j4
```

## Notes

1. The CMake build is an alternative to the Autotools build, not a replacement
2. Both build systems produce the same `circle` executable in `../bin/`
3. CMake enforces C90 standard compliance more strictly
4. MySQL configuration still uses `lib/mysql_config` file

## See Also

- [Setup and Build Guide](guides/SETUP_AND_BUILD_GUIDE.md) - Traditional build process
- [Developer Guide](DEVELOPER_GUIDE_AND_API.md) - Coding standards
- [Testing Guide](TESTING_GUIDE.md) - Running tests