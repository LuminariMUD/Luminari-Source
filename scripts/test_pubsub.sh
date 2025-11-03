#!/bin/bash

# PubSub System Test Script
echo "=== PubSub System Test Suite ==="
echo "Testing unified PubSub system functionality..."

# Test 1: Check if binary compiles and links correctly
echo ""
echo "Test 1: Compilation Test"
cd /home/jamie/Luminari-Source
if make clean && make -j20 > /dev/null 2>&1; then
    echo "✅ PASS: System compiles successfully"
else
    echo "❌ FAIL: Compilation failed"
    exit 1
fi

# Test 2: Check binary exists and is executable
echo ""
echo "Test 2: Binary Test"
if [ -x "./circle" ]; then
    echo "✅ PASS: Binary exists and is executable"
else
    echo "❌ FAIL: Binary not found or not executable"
    exit 1
fi

# Test 3: Check for symbol conflicts (no duplicate symbols)
echo ""
echo "Test 3: Symbol Conflict Test"
nm circle | grep pubsub_db_create_tables | wc -l > /tmp/symbol_count
SYMBOL_COUNT=$(cat /tmp/symbol_count)
if [ "$SYMBOL_COUNT" -eq 1 ]; then
    echo "✅ PASS: No duplicate symbols found"
else
    echo "❌ FAIL: Found $SYMBOL_COUNT instances of pubsub_db_create_tables (should be 1)"
fi

# Test 4: Check unified header structure
echo ""
echo "Test 4: Header Structure Test"
if grep -q "pubsub_message_type" src/pubsub.h && \
   grep -q "pubsub_message_category" src/pubsub.h && \
   grep -q "content_encoding" src/pubsub.h; then
    echo "✅ PASS: Unified header contains V3 enhanced structures"
else
    echo "❌ FAIL: Header missing V3 enhanced structures"
fi

# Test 5: Check for removed V3 files
echo ""
echo "Test 5: V3 Files Cleanup Test"
if [ ! -f "src/pubsub_v3.c" ] && [ ! -f "src/pubsub_v3.h" ]; then
    echo "✅ PASS: V3 files successfully removed"
else
    echo "❌ FAIL: V3 files still present"
fi

# Test 6: Check validation functions exist
echo ""
echo "Test 6: Validation Functions Test"
if grep -q "pubsub_is_valid_message_type" src/pubsub.c && \
   grep -q "pubsub_is_valid_message_category" src/pubsub.c && \
   grep -q "pubsub_is_valid_type_category_combo" src/pubsub.c; then
    echo "✅ PASS: Enhanced validation functions present"
else
    echo "❌ FAIL: Validation functions missing"
fi

# Test 7: Check database functions
echo ""
echo "Test 7: Database Functions Test"
if grep -q "pubsub_db_create_tables" src/pubsub_db.c && \
   ! grep -q "pubsub_db_create_tables" src/pubsub.c; then
    echo "✅ PASS: Database functions properly separated"
else
    echo "❌ FAIL: Database function organization issues"
fi

# Test 8: Check Makefile cleanup
echo ""
echo "Test 8: Makefile Cleanup Test"
if ! grep -q "pubsub_v3.c" Makefile.am; then
    echo "✅ PASS: Makefile cleaned of V3 references"
else
    echo "❌ FAIL: Makefile still references V3 files"
fi

echo ""
echo "=== Test Summary ==="
echo "All critical tests completed. If all show PASS, the unified system is ready!"
