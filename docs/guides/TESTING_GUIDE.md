# LuminariMUD Testing Guide

## Overview

This document describes the testing infrastructure and practices for the LuminariMUD codebase. The project uses multiple testing frameworks and approaches to ensure code quality and reliability.

## Testing Frameworks

### 1. CuTest Framework
Located in `unittests/CuTest/`, this is a lightweight C unit testing framework used for core system testing.

**Key Features:**
- Simple assertion macros
- Automatic test discovery
- Minimal setup required
- C-compatible

**Usage Example:**
```c
#include "CuTest.h"

void TestSomething(CuTest* tc) {
    int expected = 5;
    int actual = some_function();
    CuAssertIntEquals(tc, expected, actual);
}

CuSuite* GetSuite() {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, TestSomething);
    return suite;
}
```

### 2. MinUnit Framework
A minimal unit testing framework available in `unittests/minunit.h`.

**Features:**
- Header-only implementation
- Simple macros for assertions
- Lightweight and fast

### 3. Custom Test Suites
Individual modules may have their own specialized test suites.

## Module-Specific Testing

### Performance Monitor Testing

The performance monitoring system (`perfmon.c`) includes a comprehensive test suite:

**Test File:** `test_perfmon.c`

**Test Coverage:**
- Basic PerfIntvlData operations
- Circular buffer behavior
- Data aggregation between levels
- Memory management (leak detection)
- Report generation and formatting
- Buffer safety and bounds checking
- Input validation and error handling
- Pulse logging functionality
- Threshold tracking accuracy

**Running Tests:**
```bash
# Build and run perfmon tests
make test-perfmon

# Clean test artifacts
make clean-test-perfmon
```

**Test Categories:**
1. **Functional Tests** - Verify correct behavior
2. **Safety Tests** - Check buffer overflows and memory leaks
3. **Performance Tests** - Validate optimization improvements
4. **Integration Tests** - Test interaction with other systems

## Running Tests

### Individual Test Suites
```bash
# Performance monitor tests
make test-perfmon

# CuTest framework tests
cd unittests/CuTest
make
./test_runner
```

### Memory Testing
For memory leak detection, use valgrind:
```bash
# Run with memory checking
valgrind --leak-check=full --show-leak-kinds=all ./test_perfmon

# Check for buffer overflows
valgrind --tool=memcheck --track-origins=yes ./test_perfmon
```

## Writing New Tests

### Guidelines
1. **Test Naming**: Use descriptive names that indicate what is being tested
2. **Test Structure**: Follow Arrange-Act-Assert pattern
3. **Coverage**: Aim for both positive and negative test cases
4. **Independence**: Tests should not depend on each other
5. **Cleanup**: Always clean up resources in tests

### Example Test Structure
```c
static void TestFeatureName()
{
    // Arrange - Set up test data
    SomeStruct* data = create_test_data();
    
    // Act - Execute the function being tested
    int result = function_under_test(data);
    
    // Assert - Verify the results
    assert(result == expected_value);
    
    // Cleanup
    cleanup_test_data(data);
}
```

### Memory Safety Testing
Always include tests for:
- Null pointer handling
- Buffer boundary conditions
- Memory leak prevention
- Resource cleanup

## Continuous Integration

### Pre-commit Testing
Before committing code:
1. Run relevant unit tests
2. Check for memory leaks with valgrind
3. Verify no new compiler warnings
4. Test with different build configurations

### Test Automation
Consider setting up automated testing for:
- Nightly builds
- Pull request validation
- Performance regression detection
- Memory leak monitoring

## Test Data Management

### Mock Objects
Use mock objects for testing components in isolation:
- Database connections
- Network sockets
- File system operations
- External dependencies

### Test Fixtures
Create reusable test fixtures for common scenarios:
- Standard player characters
- Typical room configurations
- Common object types
- Network connection states

## Performance Testing

### Benchmarking
Include performance benchmarks for critical systems:
- Main game loop timing
- Database query performance
- Memory allocation patterns
- Network I/O throughput

### Regression Testing
Monitor for performance regressions:
- Compare execution times
- Track memory usage patterns
- Monitor resource consumption
- Validate optimization improvements

## Best Practices

### Test Organization
- Group related tests together
- Use clear, descriptive test names
- Include both positive and negative cases
- Test edge cases and boundary conditions

### Error Handling
- Test error conditions explicitly
- Verify proper cleanup on failures
- Check return codes and error messages
- Test recovery mechanisms

### Documentation
- Document test purposes and expectations
- Include setup and teardown requirements
- Explain complex test scenarios
- Maintain test documentation alongside code

## Future Improvements

### Recommended Additions
1. **Automated Test Runner** - Script to run all tests
2. **Coverage Analysis** - Track test coverage metrics
3. **Performance Baselines** - Establish performance benchmarks
4. **Integration Testing** - Full system integration tests
5. **Stress Testing** - High-load scenario testing

### Testing Infrastructure
- Continuous integration setup
- Automated memory leak detection
- Performance regression alerts
- Test result reporting and tracking

---

*This testing guide should be updated as new testing practices and frameworks are adopted.*
