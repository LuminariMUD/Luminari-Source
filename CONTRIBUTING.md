# Contributing to LuminariMUD

We welcome contributions from developers, builders, and community members! This guide outlines how to contribute effectively to the LuminariMUD project.

## Table of Contents

- [Getting Started](#getting-started)
- [Types of Contributions](#types-of-contributions)
- [Development Workflow](#development-workflow)
- [Coding Standards](#coding-standards)
- [Testing Requirements](#testing-requirements)
- [Documentation Guidelines](#documentation-guidelines)
- [Community Guidelines](#community-guidelines)
- [Contributor License Agreement](#contributor-license-agreement)

## Getting Started

### Prerequisites
- **Development Environment**: Set up according to the [Setup and Build Guide](docs/SETUP_AND_BUILD_GUIDE.md)
- **Git Knowledge**: Basic understanding of Git and GitHub workflows
- **C Programming**: Familiarity with C programming language (C99 standard)
- **MUD Knowledge**: Understanding of MUD concepts and tbaMUD/CircleMUD architecture

### First Steps
1. **Fork the Repository** on GitHub
2. **Clone Your Fork** locally
3. **Set Up Development Environment** following our setup guide
4. **Join Our Community** on [Discord](https://discord.gg/Me3Tuu4)
5. **Read the Documentation** starting with the [Technical Documentation Master Index](docs/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md)

## Types of Contributions

### Code Contributions
- **Bug Fixes**: Fix existing issues and problems
- **New Features**: Implement new game mechanics or systems
- **Performance Improvements**: Optimize existing code
- **Refactoring**: Improve code structure and maintainability
- **Security Fixes**: Address security vulnerabilities

### Content Contributions
- **World Building**: Create new areas, rooms, and zones
- **Quest Design**: Develop new quests and storylines
- **NPC Creation**: Design interesting non-player characters
- **Object Design**: Create new items, weapons, and equipment
- **Script Writing**: Develop DG scripts for dynamic content

### Documentation Contributions
- **Technical Documentation**: Improve developer and system documentation
- **User Guides**: Create or update player-facing documentation
- **API Documentation**: Document functions and interfaces
- **Tutorial Creation**: Write guides for new developers or builders

### Community Contributions
- **Bug Reports**: Report issues with detailed reproduction steps
- **Feature Requests**: Suggest new features or improvements
- **Testing**: Help test new features and releases
- **Support**: Help other community members with questions

## Development Workflow

### 1. Issue Creation and Discussion
```bash
# Before starting work, create or find an issue
# Discuss your approach with maintainers
# Get approval for major changes
```

### 2. Branch Creation
```bash
# Create a feature branch from master
git checkout master
git pull origin master
git checkout -b feature/your-feature-name

# Use descriptive branch names:
# feature/combat-system-overhaul
# bugfix/memory-leak-in-combat
# docs/update-api-documentation
```

### 3. Development Process
```bash
# Make small, focused commits
git add specific-files
git commit -m "Add initiative-based combat ordering

- Implement initiative calculation based on DEX and level
- Sort combat list by initiative values
- Add initiative display in combat status"

# Push regularly to your fork
git push origin feature/your-feature-name
```

### 4. Pull Request Submission
- **Create Pull Request** with detailed description
- **Reference Related Issues** using GitHub keywords
- **Provide Testing Instructions** for reviewers
- **Include Screenshots** for UI changes
- **Update Documentation** as needed

### 5. Code Review Process
- **Automated Testing**: Ensure all tests pass
- **Peer Review**: At least one reviewer approval required
- **Address Feedback**: Respond to review comments promptly
- **Integration Testing**: Test with full codebase
- **Final Approval**: Maintainer approval before merge

## Coding Standards

### Code Style
Follow the guidelines in our [Developer Guide](docs/DEVELOPER_GUIDE_AND_API.md):

```c
// Function naming: lowercase with underscores
void perform_combat_action(struct char_data *ch);

// Variable naming: lowercase with underscores
int player_count;
struct char_data *current_character;

// Constants: uppercase with underscores
#define MAX_PLAYERS 300
#define DEFAULT_PORT 4000

// Indentation: 2 spaces (no tabs)
if (condition) {
  if (nested_condition) {
    perform_action();
  }
}
```

### Documentation Requirements
```c
/**
 * Calculate combat damage based on weapon and character stats
 *
 * @param ch The attacking character
 * @param weapon The weapon being used (NULL for unarmed)
 * @param target The target being attacked
 * @return Total damage dealt (0 if miss)
 */
int calculate_combat_damage(struct char_data *ch,
                           struct obj_data *weapon,
                           struct char_data *target);
```

### Error Handling
```c
// Always validate input parameters
if (!ch) {
  log("SYSERR: %s called with NULL character", __func__);
  return;
}

// Use appropriate log levels
log("INFO: Player %s connected", GET_NAME(ch));
log("SYSERR: Failed to load room %d", room_vnum);
```

## Testing Requirements

### Unit Testing
```bash
# Run existing tests
make cutest

# Add tests for new functionality
# Tests should be in unittests/ directory
# Follow CuTest framework conventions
```

### Integration Testing
```bash
# Build and test full system
make clean
make all

# Test with sample data
# Verify no memory leaks with valgrind
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ../bin/circle
```

### Performance Testing
```bash
# Monitor performance with built-in tools
# Check affect_update() optimization logs
# Use performance monitoring features
```

### Manual Testing
- **Functionality Testing**: Verify features work as intended
- **Edge Case Testing**: Test boundary conditions and error cases
- **Performance Testing**: Ensure changes don't degrade performance
- **Compatibility Testing**: Test with existing content and systems

## Documentation Guidelines

### Technical Documentation
- **Update Relevant Docs**: Modify documentation for changed systems
- **API Documentation**: Document all public functions and interfaces
- **Architecture Changes**: Update system documentation for structural changes
- **Configuration Changes**: Document new configuration options

### Code Comments
```c
// Single line comments for simple explanations
int damage = calculate_base_damage(weapon);

/* Multi-line comments for complex logic
 * This section handles the complex interaction between
 * multiple combat modifiers and special abilities
 */
if (has_special_ability(ch, ABILITY_POWER_ATTACK)) {
  // Implementation details...
}
```

### Commit Messages
```bash
# Format: <type>: <description>
#
# <optional body>
#
# <optional footer>

git commit -m "feat: add initiative-based combat system

Implements D&D 3.5 style initiative ordering for combat.
Characters roll 1d20 + DEX modifier + misc bonuses.
Combat proceeds in initiative order each round.

Closes #123"
```

## Community Guidelines

### Communication Standards
- **Be Respectful**: Treat all community members with respect and kindness
- **Be Constructive**: Provide helpful, actionable feedback
- **Be Patient**: Remember that contributors have varying experience levels
- **Be Inclusive**: Welcome newcomers and help them get started

### Code of Conduct
We follow the [Contributor Covenant Code of Conduct](CODE_OF_CONDUCT.md). By participating, you agree to uphold this code.

### Getting Help
- **Discord**: Ask questions in our [Discord server](https://discord.gg/Me3Tuu4)
- **GitHub Issues**: Use for bug reports and feature requests
- **Documentation**: Check our comprehensive documentation first
- **Mentorship**: Experienced contributors are available to help newcomers

## Contributor License Agreement

### License Terms
Contributions to this project must be accompanied by a Contributor License Agreement. You (or your employer) retain the copyright to your contribution; this gives us permission to use and redistribute your contributions as part of the project.

### Dual Licensing
- **tbaMUD/CircleMUD Code**: Follows their licensing terms
- **LuminariMUD Custom Code**: Released into the public domain

### Submission Process
You generally only need to submit a CLA once. If you've already submitted one for this project, you don't need to do it again.

## Review Process

### Automated Checks
- **Build Verification**: Code must compile without errors
- **Test Execution**: All existing tests must pass
- **Code Analysis**: Static analysis tools check for common issues
- **Documentation**: Documentation must be updated for changes

### Human Review
- **Code Quality**: Reviewers check for adherence to coding standards
- **Functionality**: Verify that changes work as intended
- **Architecture**: Ensure changes fit well with existing systems
- **Security**: Check for potential security issues

### Approval Requirements
- **Peer Review**: At least one approved review from a contributor
- **Maintainer Approval**: Final approval from a project maintainer
- **Testing**: All tests must pass
- **Documentation**: Required documentation must be complete

## Recognition

### Contributors
All contributors are recognized in our project documentation and release notes.

### Maintainers
Active contributors may be invited to become maintainers with additional privileges and responsibilities.

### Community Leaders
Outstanding community members may be recognized as community leaders to help guide project direction.

---

## Quick Reference

### Useful Commands
```bash
# Setup development environment
git clone https://github.com/YOUR_USERNAME/Luminari-Source.git
cd Luminari-Source
make

# Run tests
make cutest

# Check for memory leaks
valgrind --leak-check=full ../bin/circle

# Format code (if available)
clang-format -i *.c *.h
```

### Important Links
- **[Technical Documentation](docs/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md)**
- **[Developer Guide](docs/DEVELOPER_GUIDE_AND_API.md)**
- **[Setup Guide](docs/SETUP_AND_BUILD_GUIDE.md)**
- **[AI Assistant Guide](CLAUDE.md)** - Comprehensive guide for AI-assisted development
- **[Discord Community](https://discord.gg/Me3Tuu4)**
- **[GitHub Repository](https://github.com/LuminariMUD/Luminari-Source)**

Thank you for contributing to LuminariMUD! Your efforts help make this project better for everyone.
