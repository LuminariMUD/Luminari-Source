# Narrative Weaver System Architecture Overview

**Date**: August 19, 2025  
**Purpose**: Document the complete AI-driven content creation and utilization pipeline

## Content Flow Architecture

### 1. Procedural Description Generation
- **Input**: Real-time game state (sector, lighting, resource levels, time, weather, season)
- **Process**: System generates base descriptions for wilderness rooms based on current conditions
- **Output**: Dynamic descriptions that reflect actual game world state

### 2. AI Agent Metadata Creation
- **Input**: Regional information and characteristics
- **Process**: AI agents analyze regions and generate contextual hints and sophisticated metadata
- **Output**: Categorized hints with weighting coefficients stored in database (region_hints, region_profiles)

### 3. Narrative Weaver Enhancement
- **Input**: Procedurally generated descriptions + regional metadata + current conditions
- **Process**: Weaves regional hints into base descriptions to create regional atmosphere
- **Output**: Enhanced descriptions that transform regions into unique experiences for players

### 4. Builder Override System
- **Static Rooms**: Builders can create custom descriptions for specific wilderness locations
- **Procedural Rooms**: Builders can keep generated descriptions (enhanced by narrative weaver)
- **Non-Regional**: Areas without region hints display standard generated descriptions

## Role Clarification

### Procedural Generation System
- Generates dynamic wilderness descriptions based on real-time game state
- Considers sector type, lighting conditions, resource availability, time, weather, season
- Creates base descriptions that reflect actual world conditions
- **Important**: No builder involvement in wilderness description creation (unless overridden)

### AI Agents (Regional Metadata Creation)
- Generate contextual hints and sophisticated metadata for regions
- Create weighting coefficients for seasonal and temporal relevance
- Provide quality scoring and regional character profiles
- **Important**: Focus on regional atmosphere and enhancement metadata, not base descriptions

### Narrative Weaver (Regional Enhancement)
- Takes procedurally generated wilderness descriptions as foundation
- Weaves regional hints and atmosphere into base descriptions
- Transforms regions into unique, memorable experiences for players
- **Important**: Only operates on locations within defined regions with available hints

### Builder Control Points
- **Static Room Override**: Builders can create custom descriptions for specific wilderness locations
- **Regional Configuration**: Builders can configure which areas use regional enhancement
- **Hint Management**: Builders can approve/modify AI-generated regional hints
- **Fallback Behavior**: Areas without regional hints display standard procedural descriptions
