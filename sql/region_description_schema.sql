-- Region Description Schema Extension
-- Adds comprehensive description fields to region_data table
-- Date: August 18, 2025
-- Purpose: Support template-free narrative weaving system

-- Add new columns to region_data table for comprehensive descriptions
ALTER TABLE region_data 
ADD COLUMN region_description LONGTEXT DEFAULT NULL COMMENT 'Comprehensive description for AI context',
ADD COLUMN description_version INT DEFAULT 1 COMMENT 'Version tracking for description updates',
ADD COLUMN ai_agent_source VARCHAR(100) DEFAULT NULL COMMENT 'Which AI agent created/updated description',
ADD COLUMN last_description_update TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last modification timestamp',
ADD COLUMN description_style ENUM('poetic', 'practical', 'mysterious', 'dramatic', 'pastoral') DEFAULT 'poetic' COMMENT 'Writing style for consistency',
ADD COLUMN description_length ENUM('brief', 'moderate', 'detailed', 'extensive') DEFAULT 'moderate' COMMENT 'Target description length',
ADD COLUMN has_historical_context BOOLEAN DEFAULT FALSE COMMENT 'Contains historical information',
ADD COLUMN has_resource_info BOOLEAN DEFAULT FALSE COMMENT 'Contains resource availability info',
ADD COLUMN has_wildlife_info BOOLEAN DEFAULT FALSE COMMENT 'Contains wildlife descriptions',
ADD COLUMN has_geological_info BOOLEAN DEFAULT FALSE COMMENT 'Contains geological details',
ADD COLUMN has_cultural_info BOOLEAN DEFAULT FALSE COMMENT 'Contains cultural/mystical elements',
ADD COLUMN description_quality_score DECIMAL(3,2) DEFAULT NULL COMMENT 'Quality rating 0.00-5.00',
ADD COLUMN requires_review BOOLEAN DEFAULT FALSE COMMENT 'Flagged for human review',
ADD COLUMN is_approved BOOLEAN DEFAULT FALSE COMMENT 'Approved by staff';

-- Add indexes for AI queries and performance
CREATE INDEX idx_region_has_description ON region_data (region_description(100));
CREATE INDEX idx_region_description_approved ON region_data (is_approved, requires_review);
CREATE INDEX idx_region_description_quality ON region_data (description_quality_score);
CREATE INDEX idx_region_ai_source ON region_data (ai_agent_source);
CREATE INDEX idx_region_style_length ON region_data (description_style, description_length);

-- Sample comprehensive description for The Mosswood region
INSERT INTO region_data (
    vnum, zone_vnum, name, region_type, region_props, region_reset_data,
    region_description, description_style, description_length,
    has_historical_context, has_resource_info, has_wildlife_info, 
    has_geological_info, has_cultural_info, description_quality_score,
    is_approved, ai_agent_source
) VALUES (
    1000004, 100, 'The Mosswood', 1, 0, 'standard_reset',
    'THE MOSSWOOD COMPREHENSIVE DESCRIPTION

OVERVIEW:
An ancient mystical forest spanning approximately 8 square miles, dominated by towering evergreens whose trunks are completely shrouded in thick, luminescent moss. The forest maintains an otherworldly atmosphere where bioluminescent patches create shifting patterns of ethereal green light throughout the woodland.

GEOGRAPHY & TERRAIN:
- Elevation: 150-280 feet above sea level
- Gently rolling terrain with soft, moss-covered ground
- Natural glades scattered throughout (1-2 per square mile)
- Two meandering streams with moss-covered banks
- Ancient stone formations partially reclaimed by vegetation
- Rich, dark soil with thick layers of decomposed organic matter

VEGETATION & FLORA:
Primary Canopy: Ancient pines and firs (200-400 years old), reaching 80-100 feet
Secondary Growth: Younger conifers, occasional deciduous trees
Understory: Dense fern groves, wild berry bushes, mushroom colonies
Ground Cover: Thick carpet of luminescent moss, various fungi species
Notable: Bioluminescent moss patches that glow with ethereal green light

WILDLIFE & FAUNA:
Large Mammals: Deer (common), occasional elk, rare forest spirits
Small Mammals: Rabbits, squirrels adapted to forest floor living
Birds: Owls, ravens, various forest songbirds with unusual melodic calls
Insects: Glowing beetles, phosphorescent moths, unusual butterfly species
Notable: Wildlife shows remarkable comfort with the bioluminescent environment

ATMOSPHERIC CHARACTERISTICS:
Sound: Soft whispers of wind through moss-laden branches, distant musical bird calls
Scents: Fresh pine, earthy moss, clean forest air with hints of wildflowers
Lighting: Dappled sunlight filtered through moss, ethereal green bioluminescent glow
Weather Effects: Morning mist enhances the mystical atmosphere, rain intensifies moss luminescence

MYSTICAL/CULTURAL ELEMENTS:
- Ancient legends speak of forest guardians dwelling within the moss
- Bioluminescent patterns seem to pulse in rhythm with natural cycles
- Local folklore describes the moss as having protective properties
- Travelers report feeling unusually peaceful and rejuvenated
- Some claim the forest whispers ancient secrets to those who listen

SEASONAL VARIATIONS:
Spring: Moss luminescence intensifies, new growth emerges, wildlife more active
Summer: Full canopy development, peak bioluminescent displays, comfortable temperatures
Autumn: Moss takes on golden-green hues, deciduous elements add color contrast
Winter: Snow creates stunning contrast with glowing moss, serene winter beauty

RESOURCE AVAILABILITY:
- Luminescent moss (valuable for alchemical purposes)
- Medicinal fungi and rare mushroom varieties
- Clean spring water from moss-filtered streams
- Unique pine nuts with enhanced nutritional properties
- Rare minerals found in ancient stone formations

THREATS & CONSERVATION:
- Over-harvesting of bioluminescent moss could damage ecosystem
- Climate changes affecting moss growth patterns
- Potential magical disturbances from external sources
- Need to maintain balance between accessibility and preservation

ADJACENT REGIONS:
North: Open grasslands with scattered groves
South: Wetlands transitioning to marshes
East: Rocky highlands with sparse vegetation
West: Dense ordinary forest without luminescent properties

ACCESS POINTS:
- Main trail from eastern highlands via ancient stone pathway
- Hidden passage through southern wetlands known to locals
- Northern approach through grassland clearings
- Western border marked by transition from ordinary to luminescent forest',
    'mysterious', 'extensive',
    TRUE, TRUE, TRUE, TRUE, TRUE, 4.2,
    TRUE, 'github-copilot-region-architect'
) ON DUPLICATE KEY UPDATE
    region_description = VALUES(region_description),
    description_version = description_version + 1,
    last_description_update = CURRENT_TIMESTAMP,
    ai_agent_source = VALUES(ai_agent_source);
