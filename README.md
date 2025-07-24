# LuminariMUD

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![C](https://img.shields.io/badge/language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Unix-lightgrey.svg)](https://github.com/LuminariMUD/Luminari-Source)

A text-based multiplayer online role-playing game (MUD) server implementing Pathfinder/D&D 3.5 mechanics, built on the robust tbaMUD/CircleMUD foundation with extensive custom enhancements.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Quick Start](#quick-start)
- [Installation](#installation)
- [Usage](#usage)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [Community](#community)
- [License](#license)
- [Acknowledgments](#acknowledgments)

## Overview

LuminariMUD is a feature-rich MUD server that brings the beloved Pathfinder/D&D 3.5 rule system to life in a text-based multiplayer environment. Built upon the proven tbaMUD/CircleMUD codebase, it features an original world inspired by Biblical, Dragonlance, and Forgotten Realms stories.

### Project Vision

Create a MUD with authentic Pathfinder/d20/D&D 3.5 mechanics featuring an original world that fosters a safe, friendly community for like-minded gamers. Our primary goal is building meaningful connections through collaborative storytelling and adventure.

### Project Philosophy

This project embodies commitment, self-motivation, and perseverance through challenges. Creating a MUD is inherently rewarding work, regardless of player base size. We remain dedicated to our initial vision and the hard work required to make this project successful.

## Features

### Core Game Systems
- **Authentic Pathfinder/D&D 3.5 Mechanics**: Complete implementation of familiar rule systems
- **Advanced Character System**: Multiple races, classes, feats, and skills
- **Dynamic Combat**: Initiative-based combat with tactical positioning
- **Spell System**: Comprehensive magic system with spell preparation and components
- **Crafting & Alchemy**: Extensive item creation and enhancement systems

### World & Content
- **Original World Design**: Unique setting inspired by Biblical, Dragonlance, and Forgotten Realms
- **Quest-Driven Progression**: Story-oriented advancement system
- **Living World**: Heavy scripting for dynamic, responsive environments
- **Zone-to-Zone Travel**: World map navigation with vehicle support
- **High-Quality Content**: Custom zones replacing stock content

### Technical Features
- **MySQL Integration**: Persistent player data and world state
- **DG Scripting System**: Powerful scripting for NPCs, objects, and rooms
- **Online Level Creation (OLC)**: In-game world building tools
- **Advanced Networking**: Support for modern MUD protocols
- **Performance Monitoring**: Built-in profiling and debugging tools

## Quick Start

### Prerequisites
- **Operating System**: Linux or Unix-like system
- **Compiler**: GCC with C99 support
- **Database**: MySQL 5.0+ or MariaDB
- **Libraries**: libcrypt, libgd, libm, libmysqlclient

### Build and Run
```bash
# Clone the repository
git clone https://github.com/LuminariMUD/Luminari-Source.git
cd Luminari-Source

# Build the server
make

# Run the server (after configuration)
../bin/circle
```

## Installation

### System Requirements
- **Memory**: Minimum 512MB RAM (2GB+ recommended for production)
- **Storage**: 1GB+ free disk space
- **Network**: TCP/IP networking capability
- **Compiler**: GCC 4.8+ or compatible C compiler

### Dependencies

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install build-essential mysql-server libmysqlclient-dev libgd-dev
```

#### CentOS/RHEL/Fedora
```bash
sudo yum install gcc make mysql-server mysql-devel gd-devel
# or for newer versions:
sudo dnf install gcc make mysql-server mysql-devel gd-devel
```

### Detailed Installation Steps

1. **Install Dependencies** (see above for your distribution)

2. **Clone and Build**
   ```bash
   git clone https://github.com/LuminariMUD/Luminari-Source.git
   cd Luminari-Source

   # Generate build configuration
   ./configure

   # Build the main executable
   make

   # Build utilities (optional)
   make utils
   ```

3. **Database Setup**
   ```bash
   # Create MySQL database
   mysql -u root -p
   CREATE DATABASE luminari;
   CREATE USER 'luminari'@'localhost' IDENTIFIED BY 'your_password';
   GRANT ALL PRIVILEGES ON luminari.* TO 'luminari'@'localhost';
   FLUSH PRIVILEGES;
   EXIT;
   ```

4. **Configuration**
   ```bash
   # Copy example configuration files
   cp campaign.example.h campaign.h
   cp mud_options.example.h mud_options.h
   cp vnums.example.h vnums.h

   # Edit configuration files as needed
   nano campaign.h
   nano mud_options.h
   ```

5. **First Run**
   ```bash
   # Create bin directory if it doesn't exist
   mkdir -p ../bin

   # Run the server
   ../bin/circle
   ```

## Usage

### Basic Commands

#### Building and Development
```bash
# Build everything
make all

# Clean build artifacts
make clean

# Run unit tests
make cutest

# Generate dependencies
make depend
```

#### Server Management
```bash
# Start the server
../bin/circle

# Start with specific port
../bin/circle -p 4000

# Run in background
nohup ../bin/circle &
```

### Configuration Files

- **`campaign.h`**: Core game settings and world configuration
- **`mud_options.h`**: Server options and feature toggles
- **`vnums.h`**: Virtual number assignments for objects, rooms, and NPCs

### Common Use Cases

#### For Players
- Connect via telnet: `telnet your-server-ip 4000`
- Use a MUD client like MUSHclient, TinTin++, or Mudlet for enhanced experience

#### For Builders
- Use in-game OLC (Online Level Creation) commands
- Access building documentation in `/documentation/`
- Follow building standards and guidelines

#### For Developers
- Review code in modular C files
- Use the DG scripting system for advanced features
- Contribute via GitHub pull requests

## Documentation

### Technical Documentation
- **[Master Index](documentation/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md)**: Complete technical documentation overview
- **[Architecture](documentation/CORE_SERVER_ARCHITECTURE.md)**: Server architecture and design patterns
- **[Setup Guide](documentation/SETUP_AND_BUILD_GUIDE.md)**: Detailed installation and configuration
- **[Developer Guide](documentation/DEVELOPER_GUIDE_AND_API.md)**: Coding standards and API reference

### Game Documentation
- **[Combat System](documentation/COMBAT_SYSTEM.md)**: Combat mechanics and calculations
- **[Player Management](documentation/PLAYER_MANAGEMENT_SYSTEM.md)**: Character creation and progression
- **[World Simulation](documentation/WORLD_SIMULATION_SYSTEM.md)**: World systems and mechanics

### Additional Resources
- **[Testing Guide](documentation/TESTING_GUIDE.md)**: Quality assurance and testing procedures
- **[Troubleshooting](documentation/TROUBLESHOOTING_AND_MAINTENANCE.md)**: Common issues and solutions
- **[Original tbaMUD Documentation](https://github.com/LuminariMUD/LuminariMUD/tree/master/doc)**: Legacy documentation
## Contributing

We welcome contributions from developers, builders, and community members! Please read our guidelines before contributing.

### How to Contribute

1. **Fork the Repository**
   ```bash
   # Fork on GitHub, then clone your fork
   git clone https://github.com/YOUR_USERNAME/Luminari-Source.git
   cd Luminari-Source
   ```

2. **Create a Feature Branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make Your Changes**
   - Follow our coding standards (see [Developer Guide](documentation/DEVELOPER_GUIDE_AND_API.md))
   - Add tests for new functionality
   - Update documentation as needed

4. **Test Your Changes**
   ```bash
   make clean
   make all
   make cutest  # Run unit tests
   ```

5. **Submit a Pull Request**
   - Push your branch to your fork
   - Create a pull request with a clear description
   - Reference any related issues

### Contribution Guidelines

#### Code Contributions
- **Coding Standards**: Follow existing code style and conventions
- **Documentation**: Update relevant documentation for new features
- **Testing**: Include unit tests for new functionality
- **Commit Messages**: Use clear, descriptive commit messages

#### Content Contributions
- **World Building**: Follow established lore and building standards
- **Help Files**: Maintain consistency with existing help system
- **Scripts**: Use DG scripting best practices

#### Bug Reports
- Use GitHub Issues to report bugs
- Include steps to reproduce the issue
- Provide system information and error messages
- Check existing issues before creating new ones

### Development Team Structure

#### Core Development
- **Lead Programmer**: Manages code standards and development workflow
- **Game Designer**: Defines game mechanics and project direction
- **Programmers**: Implement game mechanics and features

#### Content Creation
- **World Designer**: Designs maps, zones, and building standards
- **Lore Designer**: Develops world background and stories
- **Quest Designers**: Creates quest content and rewards
- **Builders**: Creates world content, scripts, and quests
- **Lead Scripter**: Develops universal scripts and provides support

#### Community Management
- **Lead Administrator**: Manages staff and community standards
- **Administrators**: Support player relations and enforce guidelines
- **Help File Lead**: Organizes help system and documentation

### Contributor License Agreement

Contributions to this project must be accompanied by a Contributor License Agreement. You retain copyright to your contribution; this gives us permission to use and redistribute your contributions as part of the project.

## Community

### Join Our Community
- **Discord**: [Join our community](https://discord.gg/Me3Tuu4) - Primary communication hub
- **GitHub Discussions**: Use for development-related discussions
- **Issues**: Report bugs and request features

### Community Guidelines
- **Respect**: Treat all community members with respect and kindness
- **Collaboration**: Work together towards common goals
- **Constructive Feedback**: Provide helpful, actionable feedback
- **Inclusivity**: Welcome newcomers and help them get started

### Getting Help
- **Discord**: Ask questions in appropriate channels
- **Documentation**: Check our comprehensive documentation first
- **GitHub Issues**: For bug reports and feature requests
- **In-Game Help**: Use the built-in help system

## Troubleshooting

### Common Issues

#### Build Problems
```bash
# Missing dependencies
sudo apt-get install build-essential mysql-server libmysqlclient-dev libgd-dev

# Permission issues
chmod +x configure
chmod +x licheck

# Clean build
make clean && make
```

#### Runtime Issues
```bash
# Database connection problems
# Check MySQL service status
sudo systemctl status mysql

# Port already in use
# Check what's using port 4000
netstat -tulpn | grep :4000
```

#### Configuration Issues
- Verify all `.h` configuration files are properly set up
- Check file permissions on configuration files
- Ensure database credentials are correct

### Getting Support
1. Check the [Troubleshooting Guide](documentation/TROUBLESHOOTING_AND_MAINTENANCE.md)
2. Search existing GitHub Issues
3. Ask on Discord for community support
4. Create a GitHub Issue for bugs or feature requests
## License

This project uses a dual licensing approach:

### tbaMUD/CircleMUD Code
Code contributed by the tbaMUD project follows their licensing terms. See [tbamud.com](https://tbamud.com) for details.

### LuminariMUD Custom Code
Custom code developed for LuminariMUD is released into the **public domain**:

> This is free and unencumbered software released into the public domain.
>
> Anyone is free to copy, modify, publish, use, compile, sell, or distribute this software, either in source code form or as a compiled binary, for any purpose, commercial or non-commercial, and by any means.

For complete license details, see the [LICENSE](LICENSE) file.

## Acknowledgments

### Built Upon
- **[tbaMUD](https://tbamud.com)**: The base MUD codebase
- **[CircleMUD](http://www.circlemud.org)**: The original foundation
- **CWG (Copper) MUD**: Additional enhancements and features

### Inspiration
- **Biblical Stories**: Spiritual and moral themes
- **Dragonlance**: Epic fantasy elements
- **Forgotten Realms**: Rich world-building traditions

### Version Information
- **Current Version**: LuminariMUD 2.4839 (tbaMUD 3.64)
- **Repository**: https://github.com/LuminariMUD/Luminari-Source
- **Created**: July 16, 2019
- **Language**: C (C99 standard)

---

**Remember**: *The work itself is the reward. Focus on creating something meaningful for the community.*

For more information, visit our [technical documentation](documentation/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md) or join our [Discord community](https://discord.gg/Me3Tuu4).