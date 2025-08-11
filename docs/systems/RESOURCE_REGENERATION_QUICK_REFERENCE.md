# Resource Regeneration Quick Reference

## Quick Command Reference

### Player Commands
```
survey            - Show basic resource availability
survey detailed   - Show detailed resource and regeneration info
harvest <type>    - Harvest resources (triggers regeneration check)
```

### Admin Commands
```
regen stats       - Show global regeneration statistics
regen force <zone> <x> <y> - Force regeneration at coordinates
show depletion <zone> <x> <y> - Show detailed depletion data
```

## Base Regeneration Rates (per hour)

| Resource | Rate | Notes |
|----------|------|-------|
| Vegetation | 12% | Fast-growing plants |
| Herbs | 8% | Medicinal plants |
| Water | 20% | Highly weather dependent |
| Game | 6% | Animals, breeding cycles |
| Wood | 2% | Slow tree growth |
| Clay | 10% | Weather dependent |
| Stone | 0.5% | Very slow formation |
| Minerals | 0.1% | Nearly non-renewable |
| Crystal | 0.05% | Magical regeneration |
| Salt | 1% | Steady formation |

## Seasonal Multipliers

### Vegetation/Herbs
- Winter: 30% (dormancy)
- Spring: 180% (growth explosion)
- Summer: 120% (optimal conditions)  
- Autumn: 70% (decline)

### Game Animals  
- Winter: 50% (hibernation)
- Spring: 130% (breeding)
- Summer: 100% (normal)
- Autumn: 110% (fattening)

### Wood
- Winter: 80% (slow growth)
- Spring: 120% (active growth)
- Other: 100% (normal)

## Weather Effects

### Clear Weather (0-177)
- Water: 80% (evaporation)
- Game: 120% (active)
- Others: 100%

### Light Rain (178-199)
- Water/Clay: 120%
- Vegetation/Herbs: 110%
- Others: 100%

### Heavy Rain (200-224)
- Water/Clay: 150%
- Vegetation/Herbs: 130%
- Game: 80% (shelter-seeking)

### Storms (225+)
- Water/Clay: 200%
- Vegetation/Herbs: 70% (damage)
- Game: 50% (hiding)

## Example Calculations

### Spring Herbs in Heavy Rain
- Base: 8%/hour
- Season: 180% (spring)
- Weather: 130% (rain helps)
- **Final: 18.72%/hour**

### Winter Water in Clear Weather
- Base: 20%/hour  
- Season: 100% (not seasonal)
- Weather: 80% (evaporation)
- **Final: 16%/hour**

## Troubleshooting

### No Regeneration
1. Check MySQL connection
2. Verify database initialization
3. Confirm wilderness room flags

### Slow Regeneration
1. Check if in correct season
2. Verify weather conditions
3. Confirm base rates not modified

### Database Issues
1. Run `init_resource_depletion_database()`
2. Check table schema matches docs
3. Verify coordinate indexing

## Configuration Files

- **Core Logic**: `src/resource_depletion.c`
- **Modifiers**: `src/resource_system.c`  
- **Integration**: `src/handler.c`
- **Database**: `lib/misc/resource_depletion_db.sql`

## Performance Tips

- System only processes when players move
- Database queries optimized with coordinates
- No background processing overhead
- Scales with player activity, not world size

---
*For detailed documentation see: docs/systems/RESOURCE_REGENERATION_SYSTEM.md*
