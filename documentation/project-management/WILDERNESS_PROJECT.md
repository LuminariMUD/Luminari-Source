# Luminari Wilderness Editor Project

## Project Overview

The Luminari Wilderness Editor is a Python-based web application designed to provide a visual interface for creating and editing wilderness regions, paths, and landmarks in the LuminariMUD game world. The editor will work with the existing MySQL spatial database and integrate with the game's wilderness system.

## Core Requirements

### 1. Map Display and Interaction
- Display game-generated wilderness map image as base layer
- Click-to-register coordinates functionality
- Real-time coordinate display on mouse hover
- Zoom functionality (100%, 200%, etc.)
- Coordinate display adjusts to zoom level (1x1 at 100%, 2x2 at 200%)

### 2. Drawing Tools
- **Point Tool**: Place landmarks/single-room regions
- **Polygon Tool**: Draw regions (geographic, sector, encounter)
- **Linestring Tool**: Draw paths (roads, rivers, etc.)
- **Select Tool**: Click to view/edit existing features

### 3. User Interface
- Split-window design:
  - Left: Interactive map canvas
  - Right: Information/editing panel
- Tool palette for switching between drawing modes
- Layer visibility controls
- Coordinate display (current mouse position)

### 4. Editing Features
- Manual coordinate entry/modification
- Add/remove/reorder points in polygons and paths
- Automatic polygon validation (prevent self-intersecting lines)
- Support for polygon holes (interior rings)
- Bulk selection and editing

### 5. Data Management
- Local editing with preview before committing
- Version control/commit system
- Mark items for deletion (soft delete)
- Lock regions from in-game editing
- Server selection (dev/prod environments)

### 6. Authentication & Security
- Google OAuth integration
- User permission levels
- API token authentication
- Rate limiting for DDoS protection

## Technical Architecture

### Backend Stack
- **Framework**: FastAPI or Flask
- **Database**: MySQL with spatial extensions
- **ORM**: SQLAlchemy with GeoAlchemy2
- **Authentication**: OAuth2 with Google provider
- **API**: RESTful design with OpenAPI documentation

### Frontend Stack
- **Framework**: React or Vue.js
- **Map Library**: Leaflet.js or OpenLayers
- **State Management**: Redux/Vuex or Context API
- **UI Components**: Material-UI or Ant Design
- **Build Tools**: Webpack, Babel

### Database Schema
```sql
-- Matches existing game tables
region_data (
    vnum INT PRIMARY KEY,
    zone_vnum INT,
    name VARCHAR(255),
    region_type INT,
    region_polygon GEOMETRY,
    region_props INT,
    region_reset_data TEXT,
    region_reset_time INT
)

path_data (
    vnum INT PRIMARY KEY,
    zone_vnum INT,
    name VARCHAR(255),
    path_type INT,
    path_linestring GEOMETRY,
    path_props INT
)

-- Editor-specific tables
editor_sessions (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id VARCHAR(255),
    session_data JSON,
    created_at TIMESTAMP,
    updated_at TIMESTAMP
)

editor_commits (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id VARCHAR(255),
    commit_message TEXT,
    changes JSON,
    committed_at TIMESTAMP
)
```

## API Endpoints

### Authentication
- `POST /auth/login` - Initiate OAuth flow
- `GET /auth/callback` - OAuth callback
- `POST /auth/logout` - Logout user
- `GET /auth/user` - Get current user info

### Map Data
- `GET /api/map/image` - Get base map image
- `GET /api/map/regions` - Get all regions for display
- `GET /api/map/paths` - Get all paths for display
- `GET /api/map/at/{x}/{y}` - Get features at coordinates

### Region Management
- `GET /api/regions` - List all regions
- `GET /api/regions/{vnum}` - Get specific region
- `POST /api/regions` - Create new region
- `PUT /api/regions/{vnum}` - Update region
- `DELETE /api/regions/{vnum}` - Mark region for deletion

### Path Management
- `GET /api/paths` - List all paths
- `GET /api/paths/{vnum}` - Get specific path
- `POST /api/paths` - Create new path
- `PUT /api/paths/{vnum}` - Update path
- `DELETE /api/paths/{vnum}` - Mark path for deletion

### Session Management
- `GET /api/session` - Get current editing session
- `POST /api/session/save` - Save session state
- `POST /api/session/commit` - Commit changes to database
- `POST /api/session/discard` - Discard session changes

## Development Phases

### Phase 1: Core Infrastructure
1. Set up Python backend with FastAPI
2. Configure MySQL connection with spatial support
3. Implement Google OAuth authentication
4. Create basic API structure
5. Set up development environment

### Phase 2: Basic Map Viewer
1. Create web frontend framework
2. Implement map image display
3. Add zoom and pan functionality
4. Display mouse coordinates
5. Show existing regions/paths as overlays

### Phase 3: Drawing Tools
1. Implement point placement tool
2. Add polygon drawing with vertex editing
3. Create linestring drawing for paths
4. Add selection tool for existing features
5. Implement coordinate click registration

### Phase 4: Editing Interface
1. Create information panel UI
2. Implement manual coordinate editing
3. Add point reordering functionality
4. Create property editing forms
5. Implement validation rules

### Phase 5: Data Management
1. Implement session-based editing
2. Create commit/rollback functionality
3. Add change preview system
4. Implement soft delete marking
5. Create change history tracking

### Phase 6: Advanced Features
1. Add polygon hole support
2. Implement automatic polygon fixing
3. Create bulk editing tools
4. Add region locking mechanism
5. Implement collaborative editing features

### Phase 7: Deployment & Integration
1. Set up production environment
2. Configure environment switching (dev/prod)
3. Implement backup and recovery
4. Create user documentation
5. Integrate with game systems

## Region Types (from game)
- `REGION_GEOGRAPHIC` (1) - Named geographic areas
- `REGION_ENCOUNTER` (2) - Encounter spawn zones
- `REGION_SECTOR_TRANSFORM` (3) - Terrain modification
- `REGION_SECTOR` (4) - Complete terrain override

## Path Types (from game)
- `PATH_ROAD` (1) - Paved roads
- `PATH_DIRT_ROAD` (2) - Dirt roads
- `PATH_GEOGRAPHIC` (3) - Geographic features
- `PATH_RIVER` (5) - Rivers and streams
- `PATH_STREAM` (6) - Small streams

## Coordinate System
- **Range**: -1024 to +1024 (X and Y)
- **Origin**: (0,0) at map center
- **Direction**: North (+Y), South (-Y), East (+X), West (-X)

## Security Considerations
- Validate all coordinate inputs
- Sanitize polygon/linestring data
- Implement proper SQL injection prevention
- Use prepared statements for all queries
- Rate limit API endpoints
- Log all data modifications

## Performance Requirements
- Support maps up to 2048x2048 pixels
- Handle thousands of regions/paths
- Sub-second response times for queries
- Efficient spatial indexing
- Minimal memory footprint

## User Interface Mockup
```
+--------------------------------------------------+
|  Wilderness Editor                    User: Name  |
+------------------+-------------------------------+
| Tools:           | Region: Geographic Area #101   |
| [Select] [Point] | Type: [Geographic v]          |
| [Polygon] [Line] | Name: [Darkwood Forest      ] |
|                  | Props: [Forest terrain      ] |
| Layers:          |                               |
| [x] Regions      | Points (click to edit):       |
| [x] Paths        | 1. (102, 205) [Delete]        |
| [ ] Grid         | 2. (145, 210) [Delete]        |
|                  | 3. (150, 180) [Delete]        |
| Server: [Dev v]  | 4. (102, 175) [Delete]        |
|                  | [Add Point] [Auto-Fix]        |
| Zoom: [100% v]   |                               |
|                  | [Save Draft] [Commit] [Reset] |
+------------------+-------------------------------+
| Map Canvas                                       |
| (Displays map with overlays)                     |
| Current: (X: 125, Y: 200)                        |
+--------------------------------------------------+
```

## Integration with Existing Systems
- Read map images from game's map generation system
- Use existing MySQL spatial tables
- Match coordinate system with game wilderness
- Support all existing region and path types
- Maintain compatibility with game's spatial queries

## Future Enhancements
- Mobile-responsive design
- Offline editing with sync
- Collaborative real-time editing
- Automated region generation tools
- Integration with Mudlet client
- Export/import functionality
- Procedural content generation
- 3D terrain visualization

## Development Repository
- **Name**: Luminari-Wilderness-Editor
- **Structure**:
  ```
  /
  ├── backend/
  │   ├── api/
  │   ├── auth/
  │   ├── models/
  │   └── utils/
  ├── frontend/
  │   ├── src/
  │   ├── public/
  │   └── components/
  ├── database/
  │   └── migrations/
  ├── docs/
  ├── tests/
  └── docker-compose.yml
  ```

## Getting Started (Development)
1. Clone repository
2. Set up Python virtual environment
3. Install backend dependencies: `pip install -r requirements.txt`
4. Configure MySQL connection in `.env`
5. Set up Google OAuth credentials
6. Run database migrations
7. Start backend: `uvicorn main:app --reload`
8. Install frontend dependencies: `npm install`
9. Start frontend: `npm start`
10. Access at `http://localhost:3000`

## Testing Strategy
- Unit tests for API endpoints
- Integration tests for database operations
- Frontend component testing
- End-to-end testing with Cypress
- Load testing for performance validation
- Security penetration testing

## Documentation Requirements
- API documentation (OpenAPI/Swagger)
- User guide with screenshots
- Administrator manual
- Developer setup guide
- Database schema documentation
- Troubleshooting guide

---

*This project document serves as the foundation for developing the Luminari Wilderness Editor. It should be refined and expanded as development progresses and requirements become clearer.*