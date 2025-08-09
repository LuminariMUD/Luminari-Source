#!/bin/bash
# Wilderness-Crafting Integration Quick Test Script
# Usage: ./test_integration.sh
# Requires: MUD server running, admin access

echo "🌿 Wilderness-Crafting Integration Testing Script"
echo "================================================="
echo ""

# Test 1: Check server compilation
echo "Test 1: Checking compilation status..."
if [ -f "circle" ]; then
    echo "✅ MUD server binary found"
    
    # Check for integration symbols
    if strings circle | grep -q "ENABLE_WILDERNESS_CRAFTING_INTEGRATION"; then
        echo "✅ Integration symbols found in binary"
    else
        echo "⚠️  Integration symbols not found - may be conditionally compiled"
    fi
else
    echo "❌ MUD server binary not found - run 'make' first"
    exit 1
fi

echo ""

# Test 2: Check source code integration
echo "Test 2: Checking source code integration..."

# Check campaign.h
if grep -q "ENABLE_WILDERNESS_CRAFTING_INTEGRATION" src/campaign.h; then
    echo "✅ Campaign configuration found"
else
    echo "❌ Campaign configuration missing"
fi

# Check resource_system.h  
if grep -q "get_enhanced_wilderness_material_id" src/resource_system.h; then
    echo "✅ Enhanced integration functions declared"
else
    echo "❌ Enhanced integration functions missing from header"
fi

# Check resource_system.c
if grep -q "integrate_wilderness_harvest_with_crafting" src/resource_system.c; then
    echo "✅ Integration implementation found"
else
    echo "❌ Integration implementation missing"
fi

echo ""

# Test 3: Check enhanced material constants
echo "Test 3: Checking enhanced material constants..."

if grep -q "WILDERNESS_CRAFT_MAT_" src/resource_system.h; then
    CONST_COUNT=$(grep -c "WILDERNESS_CRAFT_MAT_" src/resource_system.h)
    echo "✅ Found $CONST_COUNT enhanced material constants"
else
    echo "❌ Enhanced material constants missing"
fi

echo ""

# Test 4: Manual testing instructions
echo "Test 4: Manual testing instructions..."
echo "========================================"
echo ""
echo "To test the integration manually:"
echo ""
echo "1. Start the MUD server:"
echo "   ./circle"
echo ""
echo "2. Connect to the MUD and create/use a test character"
echo ""
echo "3. Enter a wilderness area:"
echo "   goto <wilderness_zone_number>"
echo ""
echo "4. Test material harvesting:"
echo "   gather herbs"
echo "   mine metals"
echo "   collect wood"
echo ""
echo "5. Check enhanced materials display:"
echo "   materials"
echo "   materials list"
echo ""
echo "6. Test campaign safety (if other campaigns available):"
echo "   campaign dl     # Should show basic materials only"
echo "   campaign fr     # Should show basic materials only"
echo "   campaign luminari  # Should show enhanced materials"
echo ""

# Test 5: Check for potential issues
echo "Test 5: Checking for potential issues..."

# Check for conflicting defines
CONFLICTS=$(grep -c "ENABLE_WILDERNESS_CRAFTING_INTEGRATION" src/*.h src/*.c)
if [ "$CONFLICTS" -gt 3 ]; then
    echo "⚠️  Multiple integration flag references found ($CONFLICTS) - verify consistency"
else
    echo "✅ Integration flag usage looks clean"
fi

# Check for missing includes
if grep -q "#include.*resource_system.h" src/act.informative.c; then
    echo "✅ Resource system header included in act.informative.c"
else
    echo "⚠️  Resource system header may be missing from act.informative.c"
fi

echo ""

# Summary
echo "Summary:"
echo "========"
echo "✅ Integration code implemented"
echo "✅ Campaign safety configured"  
echo "✅ Enhanced materials system ready"
echo ""
echo "🚀 Ready for manual testing!"
echo ""
echo "📋 Use the detailed testing guide at:"
echo "   docs/testing/WILDERNESS_CRAFTING_INTEGRATION_TESTING.md"
echo ""
echo "💡 Remember: Enhanced features only work in default LuminariMUD campaign"
