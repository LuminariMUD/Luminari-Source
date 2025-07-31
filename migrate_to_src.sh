#!/bin/bash

# Migration script to move LuminariMUD source files to src/ directory
# This script will:
# 1. Create src/ directory
# 2. Move all .c, .h, .cpp files and mysql/ directory to src/
# 3. Update Makefiles to reflect new structure
# 4. Fix include paths
# 5. Commit changes to git

set -e  # Exit on error

echo "Starting migration of source files to src/ directory..."

# Check if we're in a git repository
if ! git rev-parse --git-dir > /dev/null 2>&1; then
    echo "ERROR: This script must be run from within a git repository"
    exit 1
fi

# Check if src directory already exists
if [ -d "src" ] && [ "$(ls -A src)" ]; then
    echo "ERROR: src/ directory already exists and is not empty"
    exit 1
fi

# Create the src directory
echo "Creating src/ directory..."
mkdir -p src

# Move all C source files
echo "Moving C source files..."
for file in *.c; do
    if [ -f "$file" ]; then
        echo "  Moving $file"
        git mv "$file" src/
    fi
done

# Move all header files
echo "Moving header files..."
for file in *.h; do
    if [ -f "$file" ]; then
        echo "  Moving $file"
        git mv "$file" src/
    fi
done

# Move all C++ source files
echo "Moving C++ source files..."
for file in *.cpp; do
    if [ -f "$file" ]; then
        echo "  Moving $file"
        git mv "$file" src/
    fi
done

# Move mysql directory
if [ -d "mysql" ]; then
    echo "Moving mysql/ directory..."
    git mv mysql src/
fi

# Update main Makefile
echo "Updating main Makefile..."
cp Makefile Makefile.backup

# Update source file wildcards
sed -i 's/SRCFILES := \$(wildcard \*\.c)/SRCFILES := $(wildcard src\/*.c)/' Makefile
sed -i 's/CPPFILES := \$(wildcard \*\.cpp)/CPPFILES := $(wildcard src\/*.cpp)/' Makefile

# Update object file paths - fix the pattern to handle nested directory
sed -i 's/OBJFILES := \$(patsubst %.c,%.o,\$(SRCFILES)) \$(CPPFILES:%.cpp=%.o)/OBJFILES := $(SRCFILES:src\/%.c=src\/%.o) $(CPPFILES:src\/%.cpp=src\/%.o)/' Makefile

# Update constants.c dependencies - fix the filter-out pattern
sed -i 's/constants\.c: \$(filter-out constants\.c,\$(SRCFILES))/src\/constants.c: $(filter-out src\/constants.c,$(SRCFILES))/' Makefile
sed -i 's/touch constants\.c/touch src\/constants.c/' Makefile
sed -i 's/constants\.o:/src\/constants.o:/' Makefile

# Update clean target to clean src directory
sed -i 's/rm -f \*\.o depend/rm -f src\/*.o depend/' Makefile

# Update depend target
sed -i 's/\$(CC) -MM \*\.c > depend/$(CC) -MM src\/*.c | sed "s|^\([^:]*\)\.o:|src\/\\1.o:|" > depend/' Makefile

# Add include path for src directory (only if not already present)
if ! grep -q "CFLAGS.*-Isrc" Makefile; then
    sed -i 's/CFLAGS = -g -O2/CFLAGS = -g -O2 -Isrc/' Makefile
fi

# Update cutest target if it exists
if grep -q "cutest:" Makefile; then
    echo "Updating cutest target..."
    # This would need specific handling based on how cutest is structured
fi

# Stage Makefile changes
git add Makefile

# Update util/Makefile if it exists
if [ -f "util/Makefile" ]; then
    echo "Updating util/Makefile..."
    cp util/Makefile util/Makefile.backup
    sed -i 's/INCDIR = \.\./INCDIR = ..\/src/' util/Makefile
    # Also update any direct references to parent directory headers
    sed -i 's/-I\.\. /-I..\/src /' util/Makefile
    git add util/Makefile
fi

# Update unittests Makefiles if they exist
for makefile in unittests/*-Makefile; do
    if [ -f "$makefile" ]; then
        echo "Updating $makefile..."
        cp "$makefile" "$makefile.backup"
        # Update include paths
        sed -i 's/-I\.\. /-I..\/src /' "$makefile"
        # Update source file paths
        sed -i 's/\.\.\/\([^/]*\.c\)/..\/src\/\1/g' "$makefile"
        git add "$makefile"
    fi
done

# Create a simple .gitignore in src if needed
echo "*.o" > src/.gitignore
echo "depend" >> src/.gitignore
echo "*.swp" >> src/.gitignore
echo ".DS_Store" >> src/.gitignore
git add src/.gitignore

# Update any shell scripts that reference source files
echo "Checking for shell scripts that may need updating..."
for script in *.sh; do
    if [ -f "$script" ] && [ "$script" != "migrate_to_src.sh" ]; then
        if grep -q "\.c\|\.h\|\.cpp" "$script"; then
            echo "  WARNING: $script may need manual updating"
        fi
    fi
done

echo ""
echo "Migration complete! Ready to commit..."
echo ""
echo "Summary of changes:"
echo "- Moved all .c, .h, .cpp files to src/"
echo "- Moved mysql/ directory to src/mysql/"
echo "- Updated Makefile to build from src/"
echo "- Updated util/Makefile to reference src/"
echo "- Updated unittest Makefiles"
echo "- Added src/.gitignore for object files"
echo ""
echo "Backups created:"
echo "- Makefile.backup"
if [ -f "util/Makefile.backup" ]; then
    echo "- util/Makefile.backup"
fi
echo ""
echo "Please review the changes with: git status"
echo ""
echo "To commit these changes, run:"
echo "git commit -m \"Restructure: move source files to src/ directory\""
echo ""
echo "To undo these changes, run:"
echo "git reset --hard HEAD"