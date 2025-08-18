#!/bin/bash

# Script to copy binaries from code/bin to live/mud/bin
# Run this script from /home/krynn/code

# Source and destination directories
SOURCE_DIR="/home/krynn/code/bin"
DEST_DIR="/home/krynn/live/mud/bin"

# Check if source directory exists
if [ ! -d "$SOURCE_DIR" ]; then
    echo "Error: Source directory $SOURCE_DIR does not exist!"
    exit 1
fi

# Create destination directory if it doesn't exist
mkdir -p "$DEST_DIR"

# Copy all files from source to destination with force overwrite
echo "Copying binaries from $SOURCE_DIR to $DEST_DIR..."
cp -f "$SOURCE_DIR"/* "$DEST_DIR"

# Check if copy was successful
if [ $? -eq 0 ]; then
    echo "Successfully copied all binaries!"
else
    echo "Error occurred during copy operation!"
    exit 1
fi

echo "/home/krynn/live/mud/"


