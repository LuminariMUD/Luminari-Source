# LuminariMUD CMake Build Guide

## Overview

This guide covers building LuminariMUD using CMake as an alternative to the traditional Autotools approach. CMake provides cross-platform build configuration, better IDE integration, and modern build system features.

**IMPORTANT**: LuminariMUD uses C90/C89 standard with GNU extensions. The CMake configuration uses `-std=gnu90` to allow C++ style comments while maintaining C90 compatibility.

## Prerequisites

- CMake 3.12 or higher
- GCC compiler with C90 support
- MariaDB development libraries (libmariadb-dev) or MySQL development libraries
- GD graphics library (optional)

## Quick Start

```bash
# Configure required headers (one-time setup)
cp src/campaign.example.h src/campaign.h
cp src/mud_options.example.h src/mud_options.h
cp src/vnums.example.h src/vnums.h

# Configure and build with CMake
cmake -S . -B build/
cmake --build build/ -j$(nproc)

# The binary will be created at: bin/circle
ls -la bin/circle
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
# Specify MariaDB paths (if not found automatically)
cmake -DMARIADB_INCLUDE_DIR=/usr/include/mariadb \
      -DMARIADB_LIBRARY=/usr/lib/libmariadb.so ..

# Or for MySQL compatibility
cmake -DMYSQL_INCLUDE_DIR=/usr/include/mysql \
      -DMYSQL_LIBRARY=/usr/lib/libmysqlclient.so ..

# Specify GD library
cmake -DGD_LIBRARY=/usr/lib/libgd.so ..
```

## Build Targets

```bash
# Build main server
cmake --build build/ --target circle

# Build specific utility (if enabled)
cmake --build build/ --target autowiz

# Build all utilities
cmake -S . -B build/ -DBUILD_UTILS=ON
cmake --build build/

# Build unit tests
cmake -S . -B build/ -DBUILD_TESTS=ON
cmake --build build/ --target cutest

# Clean build
cmake --build build/ --target clean
# Or simply:
rm -rf build/*
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

### C++ Style Comments Note

The codebase uses C++ style comments (`//`) throughout. The CMake configuration uses GNU C90 (`-std=gnu90`) which allows these comments while maintaining C90 compatibility. This is intentional and correct.

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
cp src/campaign.example.h src/campaign.h
cp src/mud_options.example.h src/mud_options.h
cp src/vnums.example.h src/vnums.h
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

1. CMake is now the recommended build system for LuminariMUD
2. Both CMake and Autotools produce the same `circle` executable in `bin/`
3. CMake uses GNU C90 (`-std=gnu90`) to allow C++ comments while maintaining C90 base
4. The build configuration no longer generates `conf.h` - all defines are passed directly to the compiler
5. MySQL configuration still uses `lib/mysql_config` file
6. Build artifacts are cleanly separated in the `build/` directory

## See Also

- [Setup and Build Guide](guides/SETUP_AND_BUILD_GUIDE.md) - Traditional build process
- [Developer Guide](DEVELOPER_GUIDE_AND_API.md) - Coding standards
- [Testing Guide](TESTING_GUIDE.md) - Running tests