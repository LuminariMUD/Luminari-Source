# Database Integration Expansion for External API and MCP Server

## Executive Summary

This document outlines a comprehensive plan for expanding LuminariMUD's database integration to make more game data available for external editors, AI agents, and the MCP (Model Context Protocol) server. The plan focuses on using the existing MySQL/MariaDB database as the integration layer while maintaining game performance and data integrity.

## Analysis Questions Answered

### 1. Is using the database as the integration layer the best option?

**YES** - The database is the optimal integration layer for the following reasons:

**Advantages:**
- **Single Source of Truth**: Centralized data storage eliminates synchronization issues
- **Existing Infrastructure**: MySQL/MariaDB already integrated with robust spatial extensions
- **Performance**: Indexed queries and optimized spatial operations 
- **Standards Compliance**: SQL provides standardized query interface
- **Security**: Database-level access controls and authentication
- **Scalability**: Can handle multiple concurrent API consumers
- **Real-time Access**: Live data without file parsing overhead

**Current Database Assets:**
- 34+ tables already implemented
- Spatial geometry support (POLYGON, LINESTRING)
- Wilderness regions and paths fully integrated
- Player data persistence system
- Object and inventory systems
- Zone and room management

**Alternative Approaches Considered:**
- File-based exports (too static, sync issues)
- Memory dumps (security risks, performance impact)
- Custom protocols (unnecessary complexity)

### 2. What kind of game data should be made available?

Based on codebase analysis, the following data categories are candidates for external access:

#### **A. World Structure Data** (High Value)
- **Zones**: Names, level ranges, builders, descriptions, flags
- **Rooms**: Coordinates, descriptions, sector types, exits, flags
- **Wilderness**: Terrain generation, coordinates, sectors
- **Regions**: Geographic areas, encounter zones, terrain modifiers
- **Paths**: Roads, rivers, trails with geometric definitions

#### **B. Game Objects** (High Value)
- **Items**: Weapons, armor, consumables with full stats
- **Object Properties**: Damage, AC, magical effects, restrictions
- **Container Data**: Capacity, contents, special properties
- **Equipment Relationships**: What can be worn where, compatibility

#### **C. Character/Mobile Data** (Medium Value)
- **NPC Templates**: Stats, abilities, equipment, behaviors
- **Player Statistics**: Levels, classes, skills (anonymized if needed)
- **Equipment Configurations**: Loadouts, optimization data

#### **D. Game Mechanics** (Medium Value)
- **Spells**: Names, levels, effects, components, casting requirements
- **Skills**: Progression trees, requirements, synergies
- **Combat Data**: Damage types, armor interactions, special abilities
- **Crafting Systems**: Materials, recipes, requirements

#### **E. Quest and Lore Data** (Lower Priority)
- **Quest Definitions**: Objectives, rewards, prerequisites
- **Dialogue Trees**: NPC conversations, branching options
- **Lore Elements**: Historical data, faction relationships

### 3. What would be a quick win with minimum work?

#### **Immediate Quick Wins (Phase 1 - Wilderness Data)**

**1. Wilderness Coordinate System**
```sql
-- Already exists, needs view creation
CREATE VIEW api_wilderness_coordinates AS 
SELECT 
    x_coord, y_coord,
    sector_type,
    elevation,
    temperature,
    moisture
FROM wilderness_cache 
WHERE is_active = 1;
```

**2. Region Data** 
```sql
-- Leverage existing region_data and region_index tables
CREATE VIEW api_regions AS
SELECT 
    r.vnum,
    r.zone_vnum,
    r.name,
    r.region_type,
    ST_AsText(ri.region_polygon) as polygon_wkt,
    r.region_props
FROM region_data r
JOIN region_index ri ON r.vnum = ri.vnum;
```

**3. Path Data**
```sql
-- Leverage existing path_data table
CREATE VIEW api_paths AS
SELECT 
    vnum,
    zone_vnum,
    name,
    path_type,
    ST_AsText(path_linestring) as path_wkt,
    path_props
FROM path_data;
```

**Benefits of This Approach:**
- **Zero Code Changes**: Uses existing database tables
- **Immediate Value**: Wilderness editor can start working today
- **Proven Data**: Already tested and validated by game systems
- **Performance**: Leverages existing spatial indexes

### 4. Comprehensive Database Expansion Plan

## Phase 1: Wilderness and Geographic Data (Quick Win)
**Timeline: 1-2 weeks**
**Effort: Low**

### Deliverables:
- API views for wilderness, regions, and paths
- Documentation for coordinate system
- Basic REST endpoints for MCP server
- Authentication layer for API access

### Implementation:
```sql
-- Create API schema for clean separation
CREATE SCHEMA luminari_api;

-- Wilderness terrain data
CREATE VIEW luminari_api.wilderness_terrain AS
SELECT 
    world.coords[0] as x_coordinate,
    world.coords[1] as y_coordinate,
    world.sector_type,
    sector_types.name as sector_name,
    zone_table.name as zone_name,
    zone_table.number as zone_vnum
FROM world 
JOIN zone_table ON world.zone = zone_table.id
JOIN sector_types ON world.sector_type = sector_types.id
WHERE world.coords[0] IS NOT NULL;

-- Region definitions with spatial data  
CREATE VIEW luminari_api.regions AS
SELECT 
    rd.vnum,
    rd.zone_vnum,
    rd.name,
    rd.region_type,
    CASE rd.region_type
        WHEN 1 THEN 'Geographic'
        WHEN 2 THEN 'Encounter'
        WHEN 3 THEN 'Sector Transform'
        WHEN 4 THEN 'Sector Override'
    END as region_type_name,
    ST_AsText(ri.region_polygon) as polygon_wkt,
    ST_Area(ri.region_polygon) as area_size,
    rd.region_props
FROM region_data rd
JOIN region_index ri ON rd.vnum = ri.vnum;

-- Path/road/river definitions
CREATE VIEW luminari_api.paths AS
SELECT 
    pd.vnum,
    pd.zone_vnum, 
    pd.name,
    pd.path_type,
    CASE pd.path_type
        WHEN 1 THEN 'Road'
        WHEN 2 THEN 'Dirt Road'
        WHEN 3 THEN 'Geographic'
        WHEN 5 THEN 'River'
        WHEN 6 THEN 'Stream'
    END as path_type_name,
    ST_AsText(pd.path_linestring) as linestring_wkt,
    ST_Length(pd.path_linestring) as path_length,
    pd.path_props
FROM path_data pd;
```

## Phase 2: Zone and Room Data
**Timeline: 2-3 weeks**
**Effort: Medium**

### Deliverables:
- Zone information API
- Room data with descriptions and exits
- Sector type definitions
- Room flag interpretations

### New Database Tables Required:
```sql
-- Room data export table
CREATE TABLE luminari_api.rooms (
    room_vnum INT PRIMARY KEY,
    zone_vnum INT,
    room_name VARCHAR(255),
    room_description TEXT,
    sector_type INT,
    sector_name VARCHAR(50),
    room_flags JSON,
    coordinates_x INT,
    coordinates_y INT,
    is_wilderness BOOLEAN,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_zone (zone_vnum),
    INDEX idx_coords (coordinates_x, coordinates_y),
    INDEX idx_sector (sector_type)
);

-- Room exits table
CREATE TABLE luminari_api.room_exits (
    id INT AUTO_INCREMENT PRIMARY KEY,
    from_room_vnum INT,
    to_room_vnum INT,
    direction ENUM('north','south','east','west','up','down','northeast','northwest','southeast','southwest'),
    exit_description TEXT,
    door_flags JSON,
    key_vnum INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_from_room (from_room_vnum),
    INDEX idx_to_room (to_room_vnum),
    INDEX idx_direction (direction)
);

-- Zone definitions
CREATE TABLE luminari_api.zones (
    zone_vnum INT PRIMARY KEY,
    zone_name VARCHAR(255),
    builders TEXT,
    min_level INT,
    max_level INT,
    zone_flags JSON,
    reset_mode INT,
    lifespan INT,
    bottom_room INT,
    top_room INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);
```

### Data Population Strategy:
```c
// Add to existing save_char() or create periodic export function
void export_world_data_to_api() {
    room_rnum room;
    zone_rnum zone;
    
    // Export zones
    for (zone = 0; zone <= top_of_zone_table; zone++) {
        export_zone_to_api(&zone_table[zone]);
    }
    
    // Export rooms  
    for (room = 0; room <= top_of_world; room++) {
        export_room_to_api(&world[room]);
    }
}
```

## Phase 3: Object and Equipment Data
**Timeline: 3-4 weeks**
**Effort: Medium-High**

### Deliverables:
- Complete item database with stats
- Equipment relationships and compatibility
- Magical properties and effects
- Crafting materials and recipes

### Database Schema:
```sql
-- Object prototypes
CREATE TABLE luminari_api.objects (
    obj_vnum INT PRIMARY KEY,
    obj_name VARCHAR(255),
    short_description VARCHAR(255),
    long_description TEXT,
    item_type ENUM('weapon','armor','container','food','drink','light','scroll','wand','staff','potion','other'),
    wear_flags JSON,
    extra_flags JSON,
    weight DECIMAL(8,2),
    value INT,
    rent_cost INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_item_type (item_type),
    INDEX idx_value (value)
);

-- Weapon statistics
CREATE TABLE luminari_api.weapon_stats (
    obj_vnum INT PRIMARY KEY,
    weapon_type VARCHAR(50),
    num_dice INT,
    size_dice INT,
    damage_bonus INT,
    hit_bonus INT,
    critical_range INT,
    critical_multiplier INT,
    damage_types JSON,
    special_properties JSON,
    FOREIGN KEY (obj_vnum) REFERENCES luminari_api.objects(obj_vnum)
);

-- Armor statistics
CREATE TABLE luminari_api.armor_stats (
    obj_vnum INT PRIMARY KEY,
    armor_class INT,
    armor_type ENUM('light','medium','heavy','shield'),
    max_dex_bonus INT,
    armor_check_penalty INT,
    spell_failure INT,
    special_properties JSON,
    FOREIGN KEY (obj_vnum) REFERENCES luminari_api.objects(obj_vnum)
);

-- Object affects (stat bonuses)
CREATE TABLE luminari_api.object_affects (
    id INT AUTO_INCREMENT PRIMARY KEY,
    obj_vnum INT,
    affect_location INT,
    affect_modifier INT,
    affect_description VARCHAR(255),
    FOREIGN KEY (obj_vnum) REFERENCES luminari_api.objects(obj_vnum),
    INDEX idx_obj_vnum (obj_vnum)
);
```

## Phase 4: Character and Mobile Data  
**Timeline: 2-3 weeks**
**Effort: Medium**

### Deliverables:
- NPC template data
- Class and race information
- Skill and spell definitions
- Combat statistics

### Privacy Considerations:
- Player data should be anonymized or aggregated
- Individual player information requires consent
- Focus on template/prototype data for external use

## Phase 5: Advanced Game Mechanics
**Timeline: 4-5 weeks**  
**Effort: High**

### Deliverables:
- Spell system data
- Combat mechanics
- Crafting system integration
- Quest framework data

## Implementation Strategy

### Database Access Layer
```sql
-- Create API user with limited permissions
CREATE USER 'luminari_api'@'%' IDENTIFIED BY 'secure_api_password';
GRANT SELECT ON luminari_api.* TO 'luminari_api'@'%';
GRANT SELECT ON region_data TO 'luminari_api'@'%';
GRANT SELECT ON region_index TO 'luminari_api'@'%'; 
GRANT SELECT ON path_data TO 'luminari_api'@'%';

-- Create API keys table
CREATE TABLE luminari_api.api_keys (
    key_id VARCHAR(64) PRIMARY KEY,
    key_name VARCHAR(255),
    permissions JSON,
    rate_limit_per_hour INT DEFAULT 1000,
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_used_at TIMESTAMP NULL,
    expires_at TIMESTAMP NULL
);
```

### MCP Server Integration Points
```python
# Example endpoint structure for MCP server
@app.get("/api/v1/wilderness/terrain")
async def get_wilderness_terrain(
    x_min: int, y_min: int, x_max: int, y_max: int,
    api_key: str = Header(...)
):
    """Get terrain data for specified coordinate bounds"""
    
@app.get("/api/v1/regions")  
async def get_regions(zone_vnum: Optional[int] = None):
    """Get all regions or regions for specific zone"""
    
@app.get("/api/v1/paths")
async def get_paths(zone_vnum: Optional[int] = None):
    """Get all paths or paths for specific zone"""
```

### Data Synchronization Strategy

**Real-time Updates:**
- Trigger-based updates for critical data
- Event-driven synchronization for world changes
- Batch processing for bulk operations

**Scheduled Exports:**
- Nightly full synchronization
- Hourly incremental updates  
- On-demand refresh capabilities

### Performance Considerations

**Database Optimization:**
- Dedicated read replicas for API access
- Materialized views for complex queries  
- Proper indexing on coordinate and spatial data
- Query result caching

**API Rate Limiting:**
- Per-key rate limits
- Geographic query size limits
- Concurrent connection limits

## Security Framework

### Authentication & Authorization
- API key-based authentication
- Role-based access control (RBAC)
- IP whitelisting for trusted consumers
- Audit logging for all API access

### Data Protection  
- No sensitive player information exposed
- Sanitized object descriptions
- Rate limiting to prevent data scraping
- SSL/TLS encryption for all connections

## Benefits and ROI

### For External Editors
- **Wilderness Editor**: Can immediately start using region and path data
- **Room Builder**: Access to zone and room templates
- **Quest Designer**: Object and NPC reference data

### For AI Agents
- **World Understanding**: Complete spatial and geographic context
- **Game Balance**: Access to item stats and combat data  
- **Content Generation**: Templates and examples for new content

### For Players
- **Enhanced Tools**: Better character planners and guides
- **Community Resources**: Wikis and databases automatically updated
- **Third-party Applications**: Mobile apps and utilities

## Risk Assessment

### Technical Risks
- **Performance Impact**: Database load from API queries
- **Data Consistency**: Sync delays between game and API
- **Schema Changes**: Game updates breaking API compatibility

### Mitigation Strategies
- Read replicas and caching layers
- Versioned API with backward compatibility
- Comprehensive testing and monitoring

### Security Risks
- **Data Exposure**: Accidental exposure of sensitive information
- **API Abuse**: Excessive queries impacting performance
- **Unauthorized Access**: Compromise of API credentials

### Mitigation Strategies  
- Careful data filtering and sanitization
- Rate limiting and monitoring
- Regular security audits and key rotation

## Success Metrics

### Phase 1 (Wilderness Quick Win)
- ✅ Wilderness editor functional within 2 weeks
- ✅ Region and path data accessible via API
- ✅ MCP server can query geographic data

### Long-term Goals
- 📊 API serving 1000+ requests/hour without performance impact
- 🔧 External tools actively using at least 5 data categories
- 🤖 AI agents successfully generating game content using API data
- 👥 Community tools and resources leveraging live game data

## Conclusion

Using the database as an integration layer is the optimal approach for exposing LuminariMUD's rich game data to external systems. The phased implementation plan prioritizes quick wins with wilderness data while building toward comprehensive game system exposure.

The existing MySQL/MariaDB infrastructure provides a solid foundation, and the wilderness system's spatial data offers immediate high-value use cases for external editors and AI agents. This approach maintains game performance while enabling powerful new development and community tools.

---

*Document Version: 1.0*  
*Last Updated: August 15, 2025*  
*Next Review: September 15, 2025*
