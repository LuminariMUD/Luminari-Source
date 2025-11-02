# PubSub Developer Quick Start Guide

**System:** LuminariMUD Publish/Subscribe Messaging System  
**Audience:** New developers joining the PubSub development team  
**Phase:** Phase 2B Complete - Ready for Phase 3 Development  
**Last Updated:** August 13, 2025

---

## 🚀 **Get Started in 5 Minutes**

This guide gets new developers productive with the LuminariMUD PubSub system quickly.

### Step 1: Clone & Build (2 minutes)

```bash
# Clone the repository
git clone <repository_url> Luminari-Source
cd Luminari-Source

# Build the system
./autorun.sh        # If needed
make clean && make -j20

# Start the server
./bin/circle 4000
```

### Step 2: Test Basic Functionality (1 minute)

```bash
# Login as an immortal character
telnet localhost 4000

# Test the system
> pubsub status
> pubsubqueue status
```

### Step 3: Test Spatial Audio (2 minutes)

```bash
# Go to wilderness
> goto 32680

# Send spatial audio message  
> pubsubqueue spatial

# Check for automatic processing
> pubsubqueue status
```

**✅ You're ready to develop!**

---

## 📁 **Project Structure**

### Core PubSub Files

```bash
src/
├── pubsub.c / pubsub.h                    # Core system & queue management
├── pubsub_commands.c / pubsub_commands.h  # Administrative commands
├── pubsub_db.c / pubsub_db.h              # Database operations
├── pubsub_handlers.c / pubsub_handlers.h  # Message handlers
├── pubsub_queue.c / pubsub_queue.h        # Queue data structures
└── pubsub_spatial.c / pubsub_spatial.h    # Spatial audio (Phase 2B)
```

### Documentation Structure

```bash
docs/project-management/pubsub/
├── README.md                              # Master overview
├── PUB_SUB_MASTER_PLAN.md                # Complete project plan
├── phases/
│   └── PHASE_2B_SPATIAL_AUDIO_COMPLETION_REPORT.md
├── technical/
│   ├── TECHNICAL_OVERVIEW.md             # Architecture & design
│   ├── API_REFERENCE.md                  # Complete API docs
│   ├── INSTALLATION_GUIDE.md             # Setup instructions
│   └── DEVELOPER_QUICK_START.md          # This guide
└── testing/
    └── PHASE_2B_TESTING_GUIDE.md         # Testing procedures
```

---

## 🎯 **Key Concepts**

### 1. Topics & Subscriptions
```c
// Creating topics
topic_create("weather_alerts", "Weather notifications");

// Managing subscriptions  
pubsub_subscribe_player(ch, "weather_alerts");
pubsub_unsubscribe_player(ch, "weather_alerts");
```

### 2. Message Publishing
```c
// Basic message publishing
struct pubsub_message_t msg = {
    .topic = "weather_alerts",
    .content = "Storm approaching from the north!",
    .priority = PRIORITY_HIGH,
    .sender_name = "Weather System"
};
pubsub_publish_message(&msg);
```

### 3. Spatial Audio (Phase 2B)
```c
// Wilderness spatial audio
pubsub_publish_wilderness_audio(
    ch,                              // Source character
    "You hear thunder in the distance",  // Message
    PRIORITY_NORMAL,                 // Priority
    SPATIAL_AUDIO_MAX_DISTANCE      // Range
);
```

### 4. Queue Processing
```c
// Automatic processing (runs every 0.75 seconds)
void pubsub_process_message_queue(void);

// Manual queue control
pubsub_queue_start();    // Enable processing
pubsub_queue_stop();     // Disable processing
pubsub_queue_clear();    // Clear all queued messages
```

---

## 🔧 **Development Workflow**

### Making Changes

1. **Understand the code**: Read relevant source files
2. **Create tests**: Add test cases for new functionality  
3. **Implement**: Write code following existing patterns
4. **Test**: Use testing guide procedures
5. **Document**: Update API reference and documentation

### Testing Changes

```bash
# Compile with debugging
make CFLAGS="-g -DDEBUG" clean && make -j20

# Run with memory checking
valgrind --leak-check=full ./bin/circle 4000

# Test spatial audio in wilderness
# Test queue processing  
# Test administrative commands
```

### Code Style Guidelines

```c
// Function naming: use pubsub_ prefix
int pubsub_your_function(struct char_data *ch);

// Variable naming: use descriptive names
struct pubsub_message_t *current_message;
int spatial_audio_distance;

// Error handling: always check return values
if (!pubsub_initialize()) {
    log_error("PUBSUB ERROR: Failed to initialize system");
    return FALSE;
}

// Memory management: use reference counting
msg->ref_count++;  // When storing reference
pubsub_message_release(msg);  // When done with reference
```

---

## 🐛 **Common Development Issues**

### Issue: Compilation Errors

```bash
# Missing includes
#include "pubsub.h"
#include "structs.h"
#include "utils.h"

# Missing function declarations
// Add to pubsub.h if creating new public functions
```

### Issue: Runtime Crashes

```bash
# Use GDB for debugging
gdb ./bin/circle
(gdb) run 4000
# When crash occurs:
(gdb) bt         # Get backtrace
(gdb) info locals  # Check variable values
```

### Issue: Memory Leaks

```c
// Always release message references
struct pubsub_message_t *msg = pubsub_create_message(...);
// ... use message ...
pubsub_message_release(msg);  // Don't forget this!

// Check reference counting
// ref_count should return to 0 when message is cleaned up
```

### Issue: Queue Problems

```bash
# Debug queue processing
> pubsubqueue status
> pubsubqueue debug

# Check logs
tail -f log/syslog | grep "PUBSUB"
```

---

## 📚 **Essential Reading**

### For New Developers
1. **Start Here:** [README.md](../README.md) - Project overview
2. **Architecture:** [TECHNICAL_OVERVIEW.md](../technical/TECHNICAL_OVERVIEW.md) - System design
3. **API Reference:** [API_REFERENCE.md](../technical/API_REFERENCE.md) - Function documentation

### For Bug Fixes
1. **Testing Guide:** [PHASE_2B_TESTING_GUIDE.md](../testing/PHASE_2B_TESTING_GUIDE.md)
2. **Installation:** [INSTALLATION_GUIDE.md](../technical/INSTALLATION_GUIDE.md)

### For New Features  
1. **Master Plan:** [PUB_SUB_MASTER_PLAN.md](../PUB_SUB_MASTER_PLAN.md) - Complete project roadmap
2. **Phase 2B Report:** [PHASE_2B_SPATIAL_AUDIO_COMPLETION_REPORT.md](../phases/PHASE_2B_SPATIAL_AUDIO_COMPLETION_REPORT.md) - What's already done

---

## 🚀 **Phase 3 Development**

Phase 2B (Spatial Audio) is **COMPLETE**. Phase 3 development is ready to begin!

### Planned Phase 3 Features

#### Message Filtering & Priority System
- Advanced message filtering by topic, content, priority
- Multiple priority levels (CRITICAL, URGENT, HIGH, NORMAL, LOW)
- Priority-based queue processing

#### Structured Messages with Metadata
- Rich message objects with custom fields
- Tagging and categorization system
- Message expiration and persistence

#### Handler Registration System
- Dynamic message handler registration
- Plugin-style architecture for extensions
- Handler priority and chaining

#### Topic Access Control & Permissions
- Topic-based permission system
- Role-based access control (RBAC)
- Administrative topic management

#### Message Persistence & Offline Delivery
- Database-backed message storage
- Offline message queuing for disconnected players
- Message history and replay functionality

### Getting Started with Phase 3

1. **Read the master plan**: Focus on "Phase 3: Advanced Features (Weeks 6-8)"
2. **Choose a feature**: Pick one feature from the Phase 3 list
3. **Design first**: Create design documents before coding
4. **Test-driven development**: Write tests before implementation
5. **Incremental delivery**: Implement in small, testable pieces

---

## 🛠️ **Development Tools**

### Debugging Tools

```bash
# Memory debugging
valgrind --leak-check=full ./bin/circle 4000

# GDB debugging  
gdb ./bin/circle
(gdb) set args 4000
(gdb) run

# Static analysis
cppcheck src/pubsub*.c

# Code formatting
indent -kr -i2 src/pubsub*.c
```

### Testing Tools

```bash
# In-game testing commands
pubsub status          # System status
pubsub enable/disable  # Toggle system
pubsubqueue status     # Queue status
pubsubqueue start/stop # Queue control

# Log monitoring
tail -f log/syslog | grep "PUBSUB"
watch -n1 "grep 'Auto-processed' log/syslog | tail -5"
```

### Performance Tools

```bash
# Monitor memory usage
ps aux | grep circle

# Monitor system performance
top -p $(pgrep circle)

# Database performance
mysql> SHOW PROCESSLIST;
mysql> EXPLAIN SELECT * FROM pubsub_topics;
```

---

## 📞 **Getting Help**

### Documentation
- **Complete API:** [API_REFERENCE.md](../technical/API_REFERENCE.md)
- **Architecture:** [TECHNICAL_OVERVIEW.md](../technical/TECHNICAL_OVERVIEW.md)  
- **Testing:** [PHASE_2B_TESTING_GUIDE.md](../testing/PHASE_2B_TESTING_GUIDE.md)

### Debugging
1. Check logs: `tail -f log/syslog | grep "PUBSUB"`
2. Test commands: `pubsub status` and `pubsubqueue status`
3. Memory check: `valgrind --leak-check=full ./bin/circle 4000`

### Code Examples
- **Core system:** `src/pubsub.c` - Main functionality
- **Spatial audio:** `src/pubsub_spatial.c` - Phase 2B implementation
- **Commands:** `src/pubsub_commands.c` - Administrative interface

---

## ✅ **Quick Checklist**

### Before You Start Developing
- [ ] Successfully built and started server
- [ ] Tested basic pubsub commands
- [ ] Tested spatial audio in wilderness
- [ ] Read project documentation
- [ ] Understand code structure

### Before Submitting Changes  
- [ ] Code compiles without warnings
- [ ] All tests pass (see testing guide)
- [ ] Memory leaks checked with valgrind
- [ ] Documentation updated if needed
- [ ] Code follows style guidelines

### Before Production Deploy
- [ ] Full testing suite completed
- [ ] Performance testing under load
- [ ] Database migration scripts (if needed)
- [ ] Rollback plan prepared
- [ ] Monitoring alerts configured

---

*Quick start guide for LuminariMUD PubSub System developers*  
*Ready to build the future of MUD communication systems!*

**Next Steps:**
1. Complete this quick start guide
2. Review Phase 3 development plans
3. Choose your first Phase 3 feature to implement
4. Start building the next generation of MUD messaging!
