# Terrain Bridge API Server Implementation Plan

**Project:** Real-time Terrain Calculation API for External Tools  
**Date:** August 15, 2025  
**Status:** Planning Phase  
**Priority:** High  

## Overview

Create a TCP socket-based API server that allows external tools (Python APIs, web services, etc.) to request real-time terrain calculations from the running LuminariMUD server. This eliminates the need for pre-caching 4.2M wilderness coordinates while providing instant access to live terrain data.

## Architecture

### Component Overview
```
┌─────────────────┐    TCP Socket    ┌──────────────────┐    Direct Calls    ┌─────────────────┐
│   Python API   │◄────818x────────►│ Terrain API      │◄──────────────────►│ Wilderness      │
│   Web Service   │    JSON Msgs     │ Server           │   Function Calls   │ Functions       │
│   External Tool │                  │ (terrain_bridge) │                    │ (existing code) │
└─────────────────┘                  └──────────────────┘                    └─────────────────┘
```

### Design Pattern
Based on proven Discord bridge architecture (`discord_bridge.c`) with these key components:
- TCP socket server (non-blocking I/O)
- JSON message protocol
- Client connection management
- Request/response handling
- Integration with game loop

## Implementation Tasks

### Phase 1: Core Infrastructure (Priority 1)

#### Task 1.1: Create Header File
**File:** `src/terrain_bridge.h`
**Dependencies:** None
**Estimated Time:** 30 minutes

```c
#ifndef __TERRAIN_BRIDGE_H__
#define __TERRAIN_BRIDGE_H__

#include "conf.h"
#include "sysdep.h"

/* Campaign-dependent port for terrain API server */
#ifdef CAMPAIGN_DL
  #define TERRAIN_API_DEFAULT_PORT 8202  /* DragonLance campaign port */
#elif defined(CAMPAIGN_FR)
  #define TERRAIN_API_DEFAULT_PORT 8192  /* Forgotten Realms campaign port */
#else
  #define TERRAIN_API_DEFAULT_PORT 8182  /* Default Luminari port */
#endif

/* Maximum clients and message sizes */
#define TERRAIN_API_MAX_CLIENTS 10
#define TERRAIN_API_MAX_MSG_SIZE 4096
#define TERRAIN_API_MAX_BATCH_SIZE 1000

/* Client connection structure */
struct terrain_api_client {
    socket_t socket;
    char input_buffer[TERRAIN_API_MAX_MSG_SIZE];
    int input_pos;
    time_t connect_time;
    int requests_processed;
};

/* Main server structure */
struct terrain_api_server {
    socket_t server_socket;
    int port;
    struct terrain_api_client *clients;
    int num_clients;
    int max_clients;
    int total_requests;
    int total_connections;
    time_t start_time;
};

/* Global server instance */
extern struct terrain_api_server *terrain_api;

/* Function prototypes */
int start_terrain_api_server(int port);
void stop_terrain_api_server(void);
void terrain_api_process(void);
void terrain_api_accept_connections(void);
void terrain_api_process_clients(void);
void terrain_api_disconnect_client(int client_index);
char *process_terrain_request(const char *json_request);
void log_terrain_api_stats(void);

#endif /* __TERRAIN_BRIDGE_H__ */
```

#### Task 1.2: Core Server Implementation
**File:** `src/terrain_bridge.c`
**Dependencies:** `terrain_bridge.h`, `wilderness.h`, `json-c library`
**Estimated Time:** 4 hours

**Key Functions to Implement:**
1. `start_terrain_api_server()` - Initialize TCP server
2. `terrain_api_accept_connections()` - Handle new client connections
3. `terrain_api_process_clients()` - Process client messages
4. `terrain_api_disconnect_client()` - Clean disconnect handling
5. `process_terrain_request()` - JSON request parsing and response

**Implementation Notes:**
- Use `fcntl()` for non-blocking sockets (like Discord bridge)
- Implement proper error handling and logging
- Use JSON-C library for message parsing
- Follow existing code patterns from `discord_bridge.c`

#### Task 1.3: JSON Message Protocol
**Dependencies:** `json-c library`
**Estimated Time:** 2 hours

**Request Format:**
```json
{
    "command": "get_terrain|get_terrain_batch|get_static_rooms|ping",
    "x": 100,
    "y": -50,
    "params": {
        "x_min": 0, "y_min": 0,
        "x_max": 10, "y_max": 10
    }
}
```

**Response Format:**
```json
{
    "success": true,
    "data": {
        "x": 100, "y": -50,
        "elevation": 45,
        "moisture": 78,
        "temperature": 12,
        "sector_type": 3,
        "sector_name": "forest",
        "weather": 1
    },
    "error": "error message if failed"
}
```

### Phase 2: Wilderness Integration (Priority 1)

#### Task 2.1: Terrain Calculation Functions
**File:** Update `src/terrain_bridge.c`
**Dependencies:** `wilderness.h`, existing terrain functions
**Estimated Time:** 2 hours

**Functions to Integrate:**
- `get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y)`
- `get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, x, y)`
- `get_temperature(NOISE_MATERIAL_PLANE_ELEV, x, y)`
- `get_sector_type(elevation, temperature, moisture)`
- `get_weather(x, y)`
- `sector_types[sector].name` for human-readable names

**Validation Rules:**
- Coordinate bounds: -1024 to +1024 (wilderness limits)
- Batch size limit: max 1000 coordinates per request
- Input sanitization for all JSON fields

#### Task 2.2: Static Room Data Access
**File:** Update `src/terrain_bridge.c`
**Dependencies:** `structs.h`, world data
**Estimated Time:** 1 hour

**Implementation:**
- Iterate through `WILD_ROOM_VNUM_START` to `WILD_ROOM_VNUM_END`
- Check `world[real_rm].coords[0/1]` for valid coordinates
- Return room vnum, coordinates, name, sector type
- Include only rooms with non-zero coordinates

### Phase 3: Game Integration (Priority 1)

#### Task 3.1: Main Game Loop Integration
**File:** `src/comm.c`
**Dependencies:** `terrain_bridge.h`
**Estimated Time:** 30 minutes

**Changes Required:**
```c
/* In game_loop() function, add after existing processing */
#ifdef USE_TERRAIN_API
    terrain_api_process();  /* Process terrain API requests */
#endif
```

**Build System Updates:**
- Add conditional compilation flag
- Update `Makefile.am` to include `terrain_bridge.c`
- Add json-c library dependency

#### Task 3.2: Admin Commands
**File:** `src/act.wizard.c`
**Dependencies:** `terrain_bridge.h`
**Estimated Time:** 1 hour

**Commands to Add:**
```c
ACMD(do_terrain_api) {
    /* Sub-commands: start, stop, status, stats */
    /* Usage: terrainapi start [port] */
    /* Usage: terrainapi status */
    /* Usage: terrainapi stats */
}
```

**Command Registration:**
- Add to command table in `interpreter.c`
- Minimum level: LVL_GRGOD (admin only)
- Help file: `lib/text/help/terrainapi.hlp`

### Phase 4: Configuration & Build (Priority 2)

#### Task 4.1: Configuration Options
**File:** `configure.ac`
**Dependencies:** autotools
**Estimated Time:** 1 hour

**Add Configure Options:**
```bash
AC_ARG_ENABLE([terrain-api],
    [AS_HELP_STRING([--enable-terrain-api], 
     [Enable terrain bridge API server])],
    [terrain_api=$enableval], [terrain_api=no])

if test "$terrain_api" = "yes"; then
    AC_DEFINE([USE_TERRAIN_API], [1], [Enable terrain API server])
    AC_CHECK_LIB([json-c], [json_object_new_object], [],
        [AC_MSG_ERROR([json-c library required for terrain API])])
fi
```

#### Task 4.2: Makefile Updates
**File:** `Makefile.am`
**Dependencies:** autotools setup
**Estimated Time:** 30 minutes

**Changes:**
```makefile
if USE_TERRAIN_API
    TERRAIN_API_SOURCES = src/terrain_bridge.c
    TERRAIN_API_LIBS = -ljson-c
else
    TERRAIN_API_SOURCES =
    TERRAIN_API_LIBS =
endif

circle_SOURCES += $(TERRAIN_API_SOURCES)
circle_LDADD += $(TERRAIN_API_LIBS)
```

#### Task 4.3: Startup Integration
**File:** `src/comm.c` (main function)
**Dependencies:** Configuration system
**Estimated Time:** 30 minutes

**Auto-start Option:**
- Add config file setting: `terrain_api_port = 8182` (or campaign-specific)
- Add command line option: `--terrain-api-port 8182`
- Auto-start during server initialization if configured

### Phase 5: Security & Safety (Priority 2)

#### Task 5.1: Security Measures
**Dependencies:** Core implementation
**Estimated Time:** 2 hours

**Security Features:**
- Bind to localhost only by default
- Connection rate limiting (max 5 connections per second)
- Request rate limiting (max 100 requests per minute per client)
- Input validation and sanitization
- Buffer overflow protection
- Graceful handling of malformed JSON

#### Task 5.2: Error Handling & Logging
**Dependencies:** Core implementation
**Estimated Time:** 1 hour

**Logging Categories:**
- Server start/stop events
- Client connections/disconnections
- Request processing errors
- Performance statistics
- Security violations

**Error Recovery:**
- Graceful client disconnection on errors
- Server recovery from socket errors
- Memory leak prevention
- Resource cleanup on shutdown

### Phase 6: Testing & Validation (Priority 2)

#### Task 6.1: Unit Testing Framework
**File:** `unittests/test_terrain_bridge.c`
**Dependencies:** Existing unit test framework
**Estimated Time:** 3 hours

**Test Categories:**
- JSON parsing and validation
- Coordinate boundary checking
- Batch request processing
- Error handling scenarios
- Memory management
- Performance benchmarks

#### Task 6.2: Integration Testing
**Dependencies:** Complete implementation
**Estimated Time:** 2 hours

**Test Scenarios:**
- Start/stop server functionality
- Multiple concurrent clients
- Large batch requests
- Network error simulation
- Load testing (stress testing with many requests)

#### Task 6.3: Python Client Library
**File:** `util/terrain_api_client.py`
**Dependencies:** Core server implementation
**Estimated Time:** 2 hours

**Client Features:**
```python
class LuminariTerrainAPI:
    def get_terrain(self, x, y)
    def get_terrain_batch(self, x_min, y_min, x_max, y_max)
    def get_static_rooms(self)
    def ping(self)
    def get_server_stats(self)
```

**Client Testing:**
- Connection handling
- Request timeout management
- Error recovery
- Performance testing
- Example usage scripts

## Implementation Timeline

### Week 1: Core Infrastructure
- **Days 1-2:** Tasks 1.1, 1.2, 1.3 (Header, core server, JSON protocol)
- **Days 3-4:** Tasks 2.1, 2.2 (Wilderness integration, static rooms)
- **Day 5:** Task 3.1 (Game loop integration)

### Week 2: Integration & Configuration
- **Days 1-2:** Tasks 3.2, 4.1, 4.2 (Admin commands, configuration)
- **Days 3-4:** Tasks 5.1, 5.2 (Security and error handling)
- **Day 5:** Task 4.3 (Startup integration)

### Week 3: Testing & Documentation
- **Days 1-2:** Tasks 6.1, 6.2 (Unit and integration testing)
- **Days 3-4:** Task 6.3 (Python client library)
- **Day 5:** Documentation and final testing

## Resource Requirements

### Development Dependencies
- **json-c library** - JSON parsing (`sudo apt-get install libjson-c-dev`)
- **autotools** - Build system configuration
- **Python 3.x** - Client library development and testing

### System Requirements
- **Memory:** ~1MB additional RAM usage
- **CPU:** Minimal overhead (~0.1% CPU usage)
- **Network:** TCP port 8182/8192/8202 (campaign-dependent, configurable)
- **Disk:** ~50KB additional executable size

### Performance Expectations
- **Single request:** ~0.1ms processing time
- **Batch requests:** ~10ms for 100 coordinates
- **Throughput:** ~1000 requests/second theoretical max
- **Concurrent clients:** Up to 10 simultaneous connections

## Risk Assessment

### Technical Risks
- **JSON-C Dependency:** May require package installation
  - *Mitigation:* Make feature optional with configure flag
- **Socket Programming:** Complex error handling scenarios
  - *Mitigation:* Follow proven Discord bridge patterns
- **Memory Leaks:** JSON parsing and socket management
  - *Mitigation:* Comprehensive testing and valgrind verification

### Operational Risks
- **Port Conflicts:** Port 8182/8192/8202 may be in use
  - *Mitigation:* Configurable port, auto-detection
- **Security Concerns:** Network service attack surface
  - *Mitigation:* Localhost-only binding, rate limiting
- **Game Performance:** Additional processing overhead
  - *Mitigation:* Non-blocking I/O, minimal processing

## Testing Strategy

### Unit Tests
1. **JSON Protocol Testing**
   - Valid request parsing
   - Invalid JSON handling
   - Response formatting
   - Error message generation

2. **Coordinate Validation**
   - Boundary checking (-1024 to +1024)
   - Invalid coordinate handling
   - Batch size limits
   - Parameter validation

3. **Terrain Function Integration**
   - Elevation calculation accuracy
   - Moisture/temperature consistency
   - Sector type mapping
   - Weather data retrieval

### Integration Tests
1. **Socket Communication**
   - Client connection/disconnection
   - Message transmission
   - Multiple client handling
   - Network error recovery

2. **Game Loop Integration**
   - Performance impact measurement
   - Memory usage monitoring
   - Error propagation testing
   - Shutdown behavior

### Performance Tests
1. **Load Testing**
   - 100 concurrent requests
   - Large batch processing
   - Sustained request rates
   - Memory leak detection

2. **Benchmark Comparisons**
   - Direct function call timing
   - Network overhead measurement
   - Database query comparison
   - End-to-end latency testing

## Documentation Requirements

### Developer Documentation
- **API Reference:** Complete endpoint documentation
- **Integration Guide:** How to add to existing projects
- **Security Guide:** Best practices and considerations
- **Troubleshooting:** Common issues and solutions

### Administrator Documentation
- **Installation Guide:** Build and configuration steps
- **Command Reference:** Admin command usage
- **Monitoring Guide:** Performance and health monitoring
- **Security Configuration:** Access control and hardening

### User Documentation
- **Python Client Guide:** Library usage and examples
- **API Examples:** Sample requests and responses
- **Error Handling:** Client-side error management
- **Best Practices:** Efficient usage patterns

## Success Criteria

### Functional Requirements
- ✅ TCP server accepts connections on configurable port
- ✅ JSON protocol processes all defined commands
- ✅ Terrain calculations return accurate wilderness data
- ✅ Batch requests handle up to 1000 coordinates
- ✅ Admin commands provide server control and monitoring
- ✅ Python client library provides easy integration

### Performance Requirements
- ✅ Single coordinate response < 1ms processing time
- ✅ Batch requests process 100 coordinates < 10ms
- ✅ Server handles 10 concurrent clients
- ✅ No measurable impact on game performance
- ✅ Memory usage increase < 2MB

### Quality Requirements
- ✅ Zero memory leaks under normal operation
- ✅ Graceful error handling for all failure modes
- ✅ Comprehensive logging for debugging
- ✅ 95% unit test code coverage
- ✅ Complete documentation for all APIs

## Future Enhancements

### Phase 2 Features (Future Consideration)
- **Authentication System:** API key validation
- **Rate Limiting:** Per-client request throttling
- **Caching Layer:** LRU cache for frequent requests
- **WebSocket Support:** Real-time updates for client apps
- **Metrics API:** Performance and usage statistics
- **REST Interface:** HTTP endpoint alternative

### Advanced Features
- **Terrain Prediction:** Future state calculations
- **Custom Filters:** User-defined data transformations
- **Bulk Export:** Large-scale data extraction
- **Event Streaming:** Real-time wilderness changes
- **Plugin System:** Custom calculation modules

## Conclusion

The Terrain Bridge API Server provides a robust, efficient solution for external tools to access LuminariMUD's wilderness data in real-time. By leveraging proven socket infrastructure and existing terrain functions, this implementation minimizes risk while maximizing functionality.

The modular design allows for incremental development and testing, ensuring each component works correctly before integration. The comprehensive testing strategy and documentation plan support long-term maintainability and user adoption.

**Next Steps:**
1. Review and approve this implementation plan
2. Set up development environment with json-c library
3. Begin Phase 1 implementation (core infrastructure)
4. Schedule regular progress reviews and testing milestones

---
*This document will be updated as implementation progresses and requirements evolve.*
