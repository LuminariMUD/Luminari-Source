# AI Conversation History System - Implementation Plan

## Project Overview

**Objective**: Implement conversation history tracking for AI-enabled NPCs in Luminari MUD to create more engaging, context-aware NPC interactions.

**Current State**: Basic AI system exists with caching but no conversation memory between interactions.

**Target Outcome**: NPCs remember recent conversations with players, providing contextual responses that reference previous exchanges.

## Technical Foundation

### Existing AI System Architecture
- **Core Files**: `ai_service.c/h`, `ai_cache.c/h`, `ai_events.c/h`, `ai_security.c/h`
- **Cache System**: 5000 entries, 1-hour TTL, key format: `"npc_<vnum>_<input>"`
- **Integration**: `act.comm.c:545-548` triggers AI responses for MOB_AI_ENABLED NPCs
- **Character IDs**: Use `GET_MOB_VNUM()` for persistent NPC identification, `GET_IDNUM(ch)` for players

### Key Technical Requirements
- Maintain compatibility with existing cache system
- Use persistent virtual numbers (`GET_MOB_VNUM()`) not array indices
- Integrate with current async response delivery system
- Preserve session-based behavior (reset on server restart)

## Implementation Strategy

### Phase 1: Core Conversation Context (2-3 days)
**Priority**: HIGH - Immediate value with minimal risk

**Approach**: Extend existing cache system to store conversation history alongside responses.

**Key Components**:
1. **Conversation Key Generation**: Use persistent NPC virtual numbers and player IDs
2. **History Storage**: Store last 5 conversation turns in cache with same TTL as responses
3. **Context Integration**: Include conversation history in AI prompts for better responses
4. **Session Management**: Reset conversations on server restart (existing behavior)

### Phase 2: Enhanced Management (1-2 weeks)
**Priority**: MEDIUM - Quality of life improvements

**Features**:
- Conversation pruning (limit to last 10 turns)
- Admin commands for conversation inspection and management
- Memory usage monitoring and optimization
- Enhanced error handling for conversation storage

### Phase 3: Optional Analytics (2-3 weeks)
**Priority**: LOW - Data collection for future improvements

**Features**:
- MySQL table for conversation logging
- Analytics dashboard for conversation patterns
- Long-term conversation data retention
- Performance metrics and optimization insights

## Technical Implementation Details

### Core Data Structures

**Conversation Key Format**:
```c
char *make_conversation_key(struct char_data *npc, struct char_data *ch) {
    static char key[256];
    snprintf(key, sizeof(key), "conv_%d_%ld",
             GET_MOB_VNUM(npc),    // Virtual number (persistent)
             GET_IDNUM(ch));       // Player ID from char_data
    return key;
}
```

**Cache Extension**:
```c
// Extend current ai_cache_entry structure
struct ai_cache_entry {
    char *key;
    char *response;
    time_t expires_at;
    char *conversation_context;         // NEW: Recent history for prompts
    struct ai_cache_entry *next;
};
```

### Key Functions to Implement

**1. Conversation History Storage**:
```c
void ai_cache_conversation_turn(const char *base_key, const char *message, bool is_player);
```

**2. Enhanced Prompt Building**:
```c
char *build_conversation_prompt(struct char_data *npc, struct char_data *ch, const char *input);
```

**3. History Retrieval**:
```c
char *get_conversation_history(const char *base_key);
```

### Integration Points

**File**: `ai_service.c` - Function: `ai_npc_dialogue_async()`
- Add conversation turn storage before and after AI API calls
- Integrate conversation context into prompt building

**File**: `ai_cache.c` - New Functions:
- `ai_cache_conversation_turn()` - Store individual conversation turns
- `append_conversation_turn()` - Manage conversation history format
- `prune_conversation_history()` - Limit conversation length

## Performance Specifications

### Memory Usage
- **Current AI Cache**: ~10MB (5000 entries, 1-hour TTL)
- **Conversation Addition**: +5-10MB (1000 active conversations, 5 turns each)
- **Total Impact**: 50-100% increase in AI system memory usage
- **Acceptable Range**: Under 25MB total for AI system

### Response Time Targets
- **Cache Hit**: Maintain <1ms response time
- **API Call**: No change to existing 1-2 second response time
- **Conversation Lookup**: <5ms for history retrieval and formatting

### Scalability Limits
- **Concurrent Conversations**: 200-500 (limited by cache size)
- **Conversation Length**: 5-10 turns maximum (prevent memory bloat)
- **Cache Efficiency**: Maintain >70% hit ratio

## Development Roadmap

### Phase 1: Core Implementation (2-3 days)

**Day 1: Foundation**
- [ ] Create `ai_cache_conversation_turn()` function in `ai_cache.c`
- [ ] Add conversation history data structure to cache entries
- [ ] Implement conversation key generation using persistent IDs
- [ ] Add function prototypes to `ai_service.h`

**Day 2: Integration**
- [ ] Modify `ai_npc_dialogue_async()` in `ai_service.c` to store conversation turns
- [ ] Implement `build_conversation_prompt()` function
- [ ] Add conversation context to AI prompt generation
- [ ] Update response handling to store NPC replies

**Day 3: Testing & Refinement**
- [ ] Test conversation memory with multiple NPCs and players
- [ ] Verify cache memory usage stays within acceptable limits
- [ ] Ensure no regression in existing AI functionality
- [ ] Add basic error handling for conversation storage

**Deliverables**:
- NPCs remember last 5 conversation exchanges
- Conversation context included in AI prompts
- Memory usage under 25MB total
- No performance degradation

### Phase 2: Enhanced Features (1-2 weeks)

**Week 1: Management Tools**
- [ ] Implement conversation pruning (limit to 10 turns)
- [ ] Add conversation session tracking
- [ ] Create admin command: `do_ai_conversations` (list active conversations)
- [ ] Create admin command: `do_ai_history` (show conversation history)
- [ ] Create admin command: `do_ai_clear` (clear conversation cache)

**Week 2: Optimization**
- [ ] Add memory usage monitoring and alerts
- [ ] Implement conversation cleanup on player logout
- [ ] Add configuration options for conversation limits
- [ ] Enhanced error handling and logging

**Deliverables**:
- Admin tools for conversation management
- Configurable conversation limits
- Memory usage monitoring
- Robust error handling

### Phase 3: Analytics & Persistence (2-3 weeks)

**Optional Enhancement - Only if analytics needed**

**Database Schema**:
```sql
CREATE TABLE ai_conversation_history (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    mob_vnum INT NOT NULL,
    player_id BIGINT NOT NULL,
    turn_order INT NOT NULL,
    is_player BOOLEAN NOT NULL,
    message TEXT NOT NULL,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    session_start TIMESTAMP NOT NULL,

    INDEX idx_conversation (mob_vnum, player_id, session_start),
    INDEX idx_cleanup (timestamp)
);
```

**Features**:
- [ ] Async database logging of conversations
- [ ] Conversation analytics and reporting
- [ ] Long-term conversation data retention
- [ ] Web interface for conversation review

## Risk Assessment & Mitigation

### Low Risk (Phase 1) ✅
**Risks**:
- Memory usage increase (5-10MB)
- Potential cache key conflicts
- Minor performance impact

**Mitigation**:
- Monitor memory usage with alerts at 20MB threshold
- Use unique conversation key format (`conv_<vnum>_<playerid>`)
- Implement conversation pruning to limit memory growth

### Medium Risk (Phase 2) ⚠️
**Risks**:
- Thread safety issues with concurrent access
- Cache overflow with many active conversations
- Increased API token costs from longer prompts

**Mitigation**:
- Add mutex locking for cache operations
- Implement conversation limits and cleanup
- Monitor API costs and optimize prompt length

### High Risk (Phase 3) ❌
**Risks**:
- Database integration complexity
- Potential data corruption or loss
- Performance degradation from database writes

**Mitigation**:
- Thorough testing in development environment
- Implement database connection pooling
- Use async writes to prevent blocking
- Add comprehensive error handling and rollback

## Success Criteria

### Phase 1 Acceptance Criteria
- [ ] NPCs remember last 5 conversation exchanges with each player
- [ ] Conversation context appears in AI responses naturally
- [ ] Memory usage stays under 25MB total for AI system
- [ ] No performance regression in existing AI functionality
- [ ] Cache hit ratio remains above 70%

### Phase 2 Acceptance Criteria
- [ ] Admin commands work correctly for conversation management
- [ ] Conversation pruning prevents memory bloat
- [ ] Memory monitoring alerts work at configured thresholds
- [ ] Error handling gracefully manages edge cases

### Phase 3 Acceptance Criteria
- [ ] Database logging works without blocking responses
- [ ] Analytics provide useful insights into conversation patterns
- [ ] Long-term data retention works without performance impact
- [ ] Web interface provides easy conversation review

## Resource Requirements

### Development Time
- **Phase 1**: 2-3 days (1 developer)
- **Phase 2**: 1-2 weeks (1 developer)
- **Phase 3**: 2-3 weeks (1 developer + 1 database admin)

### Infrastructure
- **Phase 1**: No additional infrastructure needed
- **Phase 2**: Monitoring tools for memory usage
- **Phase 3**: Database storage expansion, web server for analytics

### Testing Requirements
- Unit tests for conversation storage functions
- Integration tests with existing AI system
- Load testing with multiple concurrent conversations
- Memory leak testing for long-running conversations

## Implementation Checklist

### Pre-Development Setup
- [ ] Review existing AI system documentation (`AI_SERVICE_README.md`)
- [ ] Analyze current cache implementation in `ai_cache.c`
- [ ] Test current AI functionality with MOB_AI_ENABLED NPCs
- [ ] Establish baseline memory usage and performance metrics

### Phase 1 Development Tasks
- [ ] Create conversation key generation function
- [ ] Implement `ai_cache_conversation_turn()` in `ai_cache.c`
- [ ] Add conversation context to cache entry structure
- [ ] Modify `ai_npc_dialogue_async()` to store conversation turns
- [ ] Implement conversation history retrieval and formatting
- [ ] Update AI prompt building to include conversation context
- [ ] Add conversation pruning to prevent memory bloat

### Testing & Validation
- [ ] Unit tests for conversation storage functions
- [ ] Integration tests with existing AI system
- [ ] Memory usage testing (target: <25MB total)
- [ ] Performance testing (maintain <1ms cache hits)
- [ ] Multi-player conversation testing
- [ ] Edge case testing (long conversations, special characters)

### Documentation & Deployment
- [ ] Update AI system documentation with conversation features
- [ ] Create admin guide for conversation management
- [ ] Add configuration options for conversation limits
- [ ] Implement monitoring and alerting for memory usage
- [ ] Create rollback plan in case of issues

## Expected Outcomes

### Immediate Benefits (Phase 1)
- **Enhanced NPC Interactions**: NPCs remember recent conversations, creating more engaging roleplay
- **Improved AI Responses**: Context-aware responses that reference previous exchanges
- **Better Player Experience**: More immersive and believable NPC behavior
- **Minimal Risk**: Built on existing cache system with proven stability

### Long-term Benefits (Phases 2-3)
- **Administrative Tools**: Easy management and monitoring of AI conversations
- **Analytics Insights**: Data-driven improvements to AI system performance
- **Scalability**: Foundation for advanced AI features like personality systems
- **Community Growth**: More engaging NPCs attract and retain players

## Conclusion

This conversation history system represents a focused, well-researched enhancement to Luminari MUD's AI capabilities. The phased approach ensures:

1. **Immediate Value**: Core functionality delivered quickly with minimal risk
2. **Incremental Improvement**: Each phase builds on previous work
3. **Technical Soundness**: Built on existing, proven systems
4. **Future Flexibility**: Foundation for advanced AI features

**Recommendation**: Proceed with Phase 1 implementation as the highest priority enhancement to the AI system. The technical approach is sound, the risk is low, and the player experience improvement will be significant.

**Next Steps**:
1. Assign developer to Phase 1 implementation
2. Set up development environment with AI system access
3. Begin with conversation key generation and cache extension
4. Test incrementally with each function addition