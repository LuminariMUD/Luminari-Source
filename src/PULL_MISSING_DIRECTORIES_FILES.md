# Strategy for Pulling Missing Directories and Files from tbaMUD

This document outlines the best approach for selectively pulling missing files and directories from the tbaMUD repository while preserving LuminariMUD's existing customizations.

## Repository Information
- **Source Repository**: https://github.com/tbamud/tbamud.git
- **Target Repository**: Current LuminariMUD repository

## Recommended Strategy: Selective Git Checkout with Comparison

This approach allows us to identify what's missing and selectively import only the files/directories we need without overwriting our customizations.

### Step 1: Setup Remote Repository

```bash
# Add tbaMUD as a remote source
git remote add tbamud https://github.com/tbamud/tbamud.git

# Fetch all branches and tags without merging
git fetch tbamud

# List available branches to choose from
git branch -r | grep tbamud
```

### Step 2: Generate File Comparison Lists

```bash
# Create a directory for comparison work
mkdir -p tmp/comparison

# Generate list of all files in tbaMUD (from their main branch)
git ls-tree -r tbamud/master --name-only | sort > tmp/comparison/tbamud-files.txt

# Generate list of all files in our repository
find . -type f -not -path "./.git/*" -not -path "./tmp/*" | sed 's|^\./||' | sort > tmp/comparison/our-files.txt

# Find files that exist in tbaMUD but not in our repo
comm -23 tmp/comparison/tbamud-files.txt tmp/comparison/our-files.txt > tmp/comparison/missing-files.txt

# Find directories that might be missing
cat tmp/comparison/missing-files.txt | xargs -I {} dirname {} | sort | uniq > tmp/comparison/missing-dirs.txt
```

### Step 3: Review and Categorize Missing Items

```bash
# Review missing files by category
echo "=== Missing Documentation ==="
grep -E "\.(md|txt|doc)$" tmp/comparison/missing-files.txt

echo "=== Missing Source Files ==="
grep -E "\.(c|h|cpp)$" tmp/comparison/missing-files.txt

echo "=== Missing Configuration ==="
grep -E "Makefile|\.in$|configure" tmp/comparison/missing-files.txt

echo "=== Missing Utilities ==="
grep -E "^util/" tmp/comparison/missing-files.txt

echo "=== Missing Library Files ==="
grep -E "^lib/" tmp/comparison/missing-files.txt
```

### Step 4: Selective Import Process

#### Option A: Import Entire Missing Directories
```bash
# For each missing directory you want to import
git checkout tbamud/master -- doc/
git checkout tbamud/master -- util/
git checkout tbamud/master -- lib/misc/
```

#### Option B: Import Specific Files
```bash
# Create a file with the specific files you want to import
cat > tmp/comparison/files-to-import.txt << 'EOF'
doc/README
util/some-utility.c
lib/misc/messages
EOF

# Import each file
while read file; do
    # Create directory if it doesn't exist
    mkdir -p "$(dirname "$file")"
    
    # Checkout the file from tbaMUD
    git checkout tbamud/master -- "$file"
    
    echo "Imported: $file"
done < tmp/comparison/files-to-import.txt
```

### Step 5: Smart Import Script

Create and run this script to intelligently import missing items:

```bash
#!/bin/bash
# File: import-missing-from-tbamud.sh

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Directories to always skip (add more as needed)
SKIP_DIRS=(
    "plrfiles"
    "syslog"
    "crash"
    "todo"
)

# File patterns to skip
SKIP_PATTERNS=(
    "*.o"
    "*.bak"
    "*~"
    ".gitignore"
    "configure"
    "config.status"
)

# Function to check if we should skip a file
should_skip() {
    local file="$1"
    
    # Check skip directories
    for dir in "${SKIP_DIRS[@]}"; do
        if [[ "$file" == "$dir/"* ]]; then
            return 0
        fi
    done
    
    # Check skip patterns
    for pattern in "${SKIP_PATTERNS[@]}"; do
        if [[ "$file" == $pattern ]]; then
            return 0
        fi
    done
    
    return 1
}

# Function to import a file with user confirmation
import_file() {
    local file="$1"
    
    if should_skip "$file"; then
        echo -e "${YELLOW}Skipping: $file${NC}"
        return
    fi
    
    echo -e "${GREEN}Found missing: $file${NC}"
    read -p "Import this file? (y/n/q): " -n 1 -r
    echo
    
    case $REPLY in
        y|Y)
            mkdir -p "$(dirname "$file")"
            git checkout tbamud/master -- "$file" 2>/dev/null
            if [ $? -eq 0 ]; then
                echo -e "${GREEN} Imported: $file${NC}"
            else
                echo -e "${RED} Failed to import: $file${NC}"
            fi
            ;;
        q|Q)
            echo "Quitting import process."
            exit 0
            ;;
        *)
            echo -e "${YELLOW}Skipped: $file${NC}"
            ;;
    esac
}

# Main import process
echo "Starting selective import from tbaMUD..."
echo "======================================="

# Process missing files
while read file; do
    import_file "$file"
done < tmp/comparison/missing-files.txt

echo "Import process completed!"
```

### Step 6: Review and Commit Changes

```bash
# Review what was imported
git status

# Diff to see the actual changes
git diff --cached

# If you need to unstage something that was imported by mistake
git reset HEAD path/to/file

# Commit the imported files
git commit -m "Import missing files/directories from tbaMUD

- Imported documentation files
- Added missing utility programs
- Included standard lib files
Source: https://github.com/tbamud/tbamud.git"
```

## Important Considerations

### 1. **Preserve Customizations**
- Never blindly import files that already exist in LuminariMUD
- Always review changes before committing
- Keep a backup branch before major imports

### 2. **Configuration Files**
- Be extra careful with Makefile, configure.ac, and other build files
- LuminariMUD likely has custom build configurations that should be preserved

### 3. **Directory Structure**
- tbaMUD structure: `src/`, `lib/`, `doc/`, `util/`
- Ensure imported files maintain the correct structure

### 4. **Version Compatibility**
- tbaMUD and LuminariMUD may have diverged significantly
- Imported files might need modifications to work with LuminariMUD's codebase

## Quick One-Liner Examples

```bash
# Import all missing documentation
git ls-tree -r tbamud/master --name-only | grep "^doc/" | xargs -I {} sh -c '[ ! -f "{}" ] && git checkout tbamud/master -- "{}"'

# Import all missing utility programs
git checkout tbamud/master -- util/

# Import specific missing lib files
git checkout tbamud/master -- lib/misc/messages lib/misc/socials

# See what world files are missing
git ls-tree -r tbamud/master --name-only | grep "^lib/world/" | while read f; do [ ! -f "$f" ] && echo "Missing: $f"; done
```

## Cleanup

After importing what you need:

```bash
# Remove the temporary comparison directory
rm -rf tmp/comparison/

# Remove the tbamud remote if no longer needed
git remote remove tbamud
```

## Summary

This strategy provides a safe, controlled way to:
1. Identify what's missing from your repository
2. Selectively import only what you need
3. Preserve your existing customizations
4. Maintain full control over the import process

The key is to never bulk-import or merge, but rather cherry-pick the specific files and directories that would benefit your project without overwriting your custom code.