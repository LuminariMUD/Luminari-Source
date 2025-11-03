#!/bin/bash
# WSL MariaDB Startup Script
# This script ensures MariaDB starts properly in WSL environment

echo "Starting MariaDB service for WSL..."

# Check if MariaDB is already running
if pgrep -x "mariadbd" > /dev/null 2>&1 || pgrep -x "mysqld" > /dev/null 2>&1; then
    echo "MariaDB is already running."
    exit 0
fi

# Create the socket directory if it doesn't exist
if [ ! -d "/run/mysqld" ]; then
    echo "Creating /run/mysqld directory..."
    sudo mkdir -p /run/mysqld
    sudo chown mysql:mysql /run/mysqld
fi

# Start MariaDB service
echo "Starting MariaDB service..."
sudo service mariadb start

# Check if service started successfully
sleep 2
if pgrep -x "mariadbd" > /dev/null 2>&1 || pgrep -x "mysqld" > /dev/null 2>&1; then
    echo "MariaDB started successfully!"
else
    echo "Failed to start MariaDB. Trying alternative method..."
    sudo service mysql start
fi