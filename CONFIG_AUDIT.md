# Configure Script Audit Report

## Executive Summary

This audit examined changes to the LuminariMUD build system that could affect the `configure` script execution. The analysis found a **CRITICAL PATH MISMATCH** issue that prevents the configure script from working correctly after the directory restructuring.

## Key Findings

### 1. **CRITICAL: Path Mismatch Issue**
The configure script expects template files in locations that no longer exist:
- Expects: `src/Makefile.in` → Actually at: `./Makefile.in`
- Expects: `src/conf.h.in` → Actually at: `./conf.h.in`
- Expects: `src/util/Makefile.in` → Actually at: `./util/Makefile.in`

**This causes configure to fail with:**
```
sed: can't read ./src/Makefile.in: No such file or directory
sed: can't read ./src/util/Makefile.in: No such file or directory
cat: ./src/conf.h.in: No such file or directory
```

### 2. **Critical Makefile.in Changes**

#### Commit 11a4c2ba (2025-08-01): Added C90 Standard Flag
```diff
-CFLAGS = @CFLAGS@ $(MYFLAGS) $(PROFILE)
+CFLAGS = @CFLAGS@ -std=gnu90 $(MYFLAGS) $(PROFILE)
```
**Impact**: Forces compilation in GNU C90 mode, which is critical for the codebase that requires C90 compliance (no C99 features).

### 3. **New Library Dependencies**

The build system now requires additional libraries not checked by configure:
```makefile
LIBS = @LIBS@ @CRYPTLIB@ @NETLIB@ -lcurl -lssl -lcrypto -lpthread
```

These were added for:
- **AI Service** (`ai_service.c`): Requires `curl` and `pthread`
- **SSL/Crypto**: Required for secure communications
- **Threading**: Added for AI service background processing

### 4. **File Relocations That Broke Configure**
During directory restructuring:
- Template files (Makefile.in, conf.h.in) remained in root directory
- But configure script still expects them in `src/` subdirectory
- This mismatch causes configure to fail when creating output files

## Recommendations

### Immediate Actions Required:

1. **FIX THE PATH MISMATCH** (Choose one approach):

   **Option A - Move template files to expected locations:**
   ```bash
   mkdir -p src
   mv Makefile.in src/
   mv conf.h.in src/
   mv util src/
   ```

   **Option B - Regenerate configure from updated configure.in:**
   ```bash
   cd cnf
   autoconf configure.in > ../configure
   ```

   **Option C - Create symbolic links (temporary fix):**
   ```bash
   mkdir -p src
   ln -s ../Makefile.in src/Makefile.in
   ln -s ../conf.h.in src/conf.h.in
   ln -s ../util src/util
   ```

2. **Install Required Libraries** before running configure:
   ```bash
   # Debian/Ubuntu
   sudo apt-get install libcurl4-openssl-dev libssl-dev libpthread-stubs0-dev
   
   # RHEL/CentOS
   sudo yum install libcurl-devel openssl-devel
   ```

3. **Then Run Configure and Build**:
   ```bash
   ./configure
   cd src
   make
   ```

### Future Improvements (Optional):

1. **Update configure.in** to check for new dependencies:
   - Add AC_CHECK_LIB for curl, ssl, crypto, pthread
   - Add AC_CHECK_HEADERS for curl/curl.h, openssl/ssl.h

2. **Modernize Autoconf**:
   - Current version (2.13) is from 1999
   - Consider updating to modern autoconf (2.71+)
   - Would require rewriting some deprecated macros

## Build Process Summary

The current build process is **BROKEN** due to path mismatches:
1. Configure will FAIL when trying to create output files
2. Template files are not where configure expects them
3. Must fix the path issue before configure can work
4. After fixing paths, compilation still requires new libraries installed
5. The `-std=gnu90` flag ensures C90 compliance

## Affected Files Timeline

- **2025-08-01**: Added `-std=gnu90` compiler flag
- **Various dates**: Added AI service with curl/pthread dependencies
- **2025-08-01**: Restructured directory layout (no functional impact)

## Conclusion

The configure script is **currently broken** for public deployment due to the directory restructuring that moved source files but not template files. The path mismatch must be fixed before users can successfully run configure. Once fixed, users will also need to install the new library dependencies (curl, ssl, crypto, pthread) before building.