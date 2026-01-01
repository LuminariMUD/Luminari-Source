-- ============================================================================
-- Rogue Assassin Tree: Tier 1 and 2 Perks Implementation
-- ============================================================================
-- This script adds the first two tiers of the Rogue Assassin perk tree
-- Total: 10 perks (4 Tier 1, 6 Tier 2)
-- Perk IDs: 300-309
-- ============================================================================

-- Class constant: CLASS_ROGUE = 2

-- ============================================================================
-- TIER 1 PERKS (1 point each)
-- ============================================================================

-- Perk ID 300: Sneak Attack I (Rank 1-5)
INSERT INTO perk_data (
    perk_id,
    name,
    description,
    associated_class,
    cost,
    max_rank,
    prerequisite_perk,
    prerequisite_rank,
    effect_type,
    effect_value,
    effect_modifier,
    special_description,
    toggleable
) VALUES (
    300,
    'Sneak Attack I',
    '+1d6 sneak attack damage per rank',
    2,  -- CLASS_ROGUE
    1,
    5,
    -1,  -- No prerequisite
    0,
    18,  -- PERK_EFFECT_SPECIAL
    1,   -- +1d6 per rank
    0,
    'Adds 1d6 sneak attack damage per rank. Can be taken 5 times for +5d6 total.',
    0
);

-- Perk ID 301: Vital Strike
INSERT INTO perk_data (
    perk_id,
    name,
    description,
    associated_class,
    cost,
    max_rank,
    prerequisite_perk,
    prerequisite_rank,
    effect_type,
    effect_value,
    effect_modifier,
    special_description,
    toggleable
) VALUES (
    301,
    'Vital Strike',
    '+2 to confirm critical hits',
    2,  -- CLASS_ROGUE
    1,
    1,
    -1,  -- No prerequisite
    0,
    18,  -- PERK_EFFECT_SPECIAL
    2,
    0,
    'Grants +2 bonus to critical hit confirmation rolls.',
    0
);

-- Perk ID 302: Deadly Aim I (Rank 1-3)
INSERT INTO perk_data (
    perk_id,
    name,
    description,
    associated_class,
    cost,
    max_rank,
    prerequisite_perk,
    prerequisite_rank,
    effect_type,
    effect_value,
    effect_modifier,
    special_description,
    toggleable
) VALUES (
    302,
    'Deadly Aim I',
    '+1 damage with ranged sneak attacks per rank',
    2,  -- CLASS_ROGUE
    1,
    3,
    -1,  -- No prerequisite
    0,
    18,  -- PERK_EFFECT_SPECIAL
    1,
    0,
    'Adds +1 damage to ranged sneak attacks per rank. Can be taken 3 times.',
    0
);

-- Perk ID 303: Opportunist I
INSERT INTO perk_data (
    perk_id,
    name,
    description,
    associated_class,
    cost,
    max_rank,
    prerequisite_perk,
    prerequisite_rank,
    effect_type,
    effect_value,
    effect_modifier,
    special_description,
    toggleable
) VALUES (
    303,
    'Opportunist I',
    '+1 attack of opportunity per round',
    2,  -- CLASS_ROGUE
    1,
    1,
    -1,  -- No prerequisite
    0,
    18,  -- PERK_EFFECT_SPECIAL
    1,
    0,
    'Grants one additional attack of opportunity per round.',
    0
);

-- ============================================================================
-- TIER 2 PERKS (2 points each)
-- ============================================================================

-- Perk ID 304: Sneak Attack II (Rank 1-3)
INSERT INTO perk_data (
    perk_id,
    name,
    description,
    associated_class,
    cost,
    max_rank,
    prerequisite_perk,
    prerequisite_rank,
    effect_type,
    effect_value,
    effect_modifier,
    special_description,
    toggleable
) VALUES (
    304,
    'Sneak Attack II',
    'Additional +1d6 sneak attack damage per rank',
    2,  -- CLASS_ROGUE
    2,
    3,
    300,  -- Requires Sneak Attack I
    5,    -- Must have max rank (5) of Sneak Attack I
    18,   -- PERK_EFFECT_SPECIAL
    1,
    0,
    'Requires Sneak Attack I at max rank. Adds additional 1d6 sneak attack damage per rank.',
    0
);

-- Perk ID 305: Improved Vital Strike
INSERT INTO perk_data (
    perk_id,
    name,
    description,
    associated_class,
    cost,
    max_rank,
    prerequisite_perk,
    prerequisite_rank,
    effect_type,
    effect_value,
    effect_modifier,
    special_description,
    toggleable
) VALUES (
    305,
    'Improved Vital Strike',
    'Additional +2 to confirm criticals (+4 total)',
    2,  -- CLASS_ROGUE
    2,
    1,
    301,  -- Requires Vital Strike
    1,
    18,   -- PERK_EFFECT_SPECIAL
    2,
    0,
    'Requires Vital Strike. Grants additional +2 bonus to critical confirmation (+4 total).',
    0
);

-- Perk ID 306: Assassinate I
INSERT INTO perk_data (
    perk_id,
    name,
    description,
    associated_class,
    cost,
    max_rank,
    prerequisite_perk,
    prerequisite_rank,
    effect_type,
    effect_value,
    effect_modifier,
    special_description,
    toggleable
) VALUES (
    306,
    'Assassinate I',
    'Sneak attacks from stealth deal +2d6 damage',
    2,  -- CLASS_ROGUE
    2,
    1,
    300,  -- Requires Sneak Attack I
    3,    -- Must have at least 3 ranks of Sneak Attack I
    18,   -- PERK_EFFECT_SPECIAL
    2,
    0,
    'Requires Sneak Attack I (at least 3 ranks). Attacks from stealth deal +2d6 additional damage.',
    0
);

-- Perk ID 307: Deadly Aim II (Rank 1-2)
INSERT INTO perk_data (
    perk_id,
    name,
    description,
    associated_class,
    cost,
    max_rank,
    prerequisite_perk,
    prerequisite_rank,
    effect_type,
    effect_value,
    effect_modifier,
    special_description,
    toggleable
) VALUES (
    307,
    'Deadly Aim II',
    'Additional +2 damage with ranged sneak attacks per rank',
    2,  -- CLASS_ROGUE
    2,
    2,
    302,  -- Requires Deadly Aim I
    3,    -- Must have max rank (3) of Deadly Aim I
    18,   -- PERK_EFFECT_SPECIAL
    2,
    0,
    'Requires Deadly Aim I at max rank. Adds +2 damage to ranged sneak attacks per rank.',
    0
);

-- Perk ID 308: Crippling Strike
INSERT INTO perk_data (
    perk_id,
    name,
    description,
    associated_class,
    cost,
    max_rank,
    prerequisite_perk,
    prerequisite_rank,
    effect_type,
    effect_value,
    effect_modifier,
    special_description,
    toggleable
) VALUES (
    308,
    'Crippling Strike',
    'Sneak attacks reduce target movement by 50% for 3 rounds',
    2,  -- CLASS_ROGUE
    2,
    1,
    300,  -- Requires Sneak Attack I
    3,    -- Must have at least 3 ranks of Sneak Attack I
    18,   -- PERK_EFFECT_SPECIAL
    50,   -- 50% movement reduction
    3,    -- Duration: 3 rounds
    'Requires Sneak Attack I (at least 3 ranks). Sneak attacks apply movement speed reduction.',
    0
);

-- Perk ID 309: Bleeding Attack
INSERT INTO perk_data (
    perk_id,
    name,
    description,
    associated_class,
    cost,
    max_rank,
    prerequisite_perk,
    prerequisite_rank,
    effect_type,
    effect_value,
    effect_modifier,
    special_description,
    toggleable
) VALUES (
    309,
    'Bleeding Attack',
    'Sneak attacks cause target to bleed for 1d6 damage per round (5 rounds)',
    2,  -- CLASS_ROGUE
    2,
    1,
    300,  -- Requires Sneak Attack I
    3,    -- Must have at least 3 ranks of Sneak Attack I
    18,   -- PERK_EFFECT_SPECIAL
    1,    -- 1d6 damage per round
    5,    -- Duration: 5 rounds
    'Requires Sneak Attack I (at least 3 ranks). Sneak attacks apply bleeding damage over time.',
    0
);

-- ============================================================================
-- Verification Queries
-- ============================================================================

-- Check all Rogue Assassin T1-T2 perks were inserted
SELECT
    perk_id,
    name,
    cost,
    max_rank,
    prerequisite_perk,
    prerequisite_rank
FROM perk_data
WHERE perk_id BETWEEN 300 AND 309
ORDER BY perk_id;

-- Show the prerequisite chain
SELECT
    p.perk_id,
    p.name,
    p.cost,
    CASE
        WHEN p.prerequisite_perk = -1 THEN 'None'
        ELSE CONCAT(pre.name, ' (rank ', p.prerequisite_rank, ')')
    END AS prerequisite
FROM perk_data p
LEFT JOIN perk_data pre ON p.prerequisite_perk = pre.perk_id
WHERE p.perk_id BETWEEN 300 AND 309
ORDER BY p.perk_id;

-- Count perks by tier (based on cost)
SELECT
    cost AS tier_cost,
    COUNT(*) AS perk_count,
    SUM(cost * max_rank) AS max_point_investment
FROM perk_data
WHERE perk_id BETWEEN 300 AND 309
GROUP BY cost
ORDER BY cost;

-- ============================================================================
-- Implementation Notes
-- ============================================================================
--
-- Tier 1 Perks (4 perks, 11 total ranks):
--   - Sneak Attack I: 5 ranks × 1 point = 5 points
--   - Vital Strike: 1 rank × 1 point = 1 point
--   - Deadly Aim I: 3 ranks × 1 point = 3 points
--   - Opportunist I: 1 rank × 1 point = 1 point
--   TOTAL TIER 1: 10 points possible
--
-- Tier 2 Perks (6 perks, 11 total ranks):
--   - Sneak Attack II: 3 ranks × 2 points = 6 points
--   - Improved Vital Strike: 1 rank × 2 points = 2 points
--   - Assassinate I: 1 rank × 2 points = 2 points
--   - Deadly Aim II: 2 ranks × 2 points = 4 points
--   - Crippling Strike: 1 rank × 2 points = 2 points
--   - Bleeding Attack: 1 rank × 2 points = 2 points
--   TOTAL TIER 2: 18 points possible
--
-- COMBINED T1+T2: 28 points possible (player needs 28 points to max both tiers)
--
-- Code Implementation Required:
-- 1. Sneak attack damage calculation (hook into combat system)
-- 2. Critical confirmation bonus (modify critical hit rolls)
-- 3. Ranged sneak attack damage bonus
-- 4. Attack of opportunity count increase
-- 5. Stealth detection for Assassinate bonus
-- 6. Movement speed reduction debuff (Crippling Strike)
-- 7. Damage over time effect (Bleeding Attack)
--
-- Testing Checklist:
-- [ ] Sneak Attack I ranks stack properly (up to +5d6)
-- [ ] Sneak Attack II requires Sneak Attack I max rank
-- [ ] Vital Strike provides +2 to critical confirmation
-- [ ] Improved Vital Strike stacks (+4 total)
-- [ ] Deadly Aim bonuses apply only to ranged attacks
-- [ ] Assassinate bonus applies when attacking from stealth
-- [ ] Crippling Strike reduces movement speed
-- [ ] Bleeding Attack applies DOT effect
-- [ ] Opportunist grants additional AoO
-- [ ] Prerequisites are enforced correctly
--
-- ============================================================================
