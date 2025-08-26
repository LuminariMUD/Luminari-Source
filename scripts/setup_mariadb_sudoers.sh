#!/bin/bash
# This script sets up passwordless sudo for MariaDB commands
# Run this once with: sudo bash setup_mariadb_sudoers.sh

echo "Setting up passwordless sudo for MariaDB startup..."

# Create a sudoers file for MariaDB commands
cat > /tmp/mariadb-sudoers << 'EOF'
# Allow passwordless sudo for MariaDB startup commands
xamgibson ALL=(ALL) NOPASSWD: /usr/bin/mkdir -p /run/mysqld
xamgibson ALL=(ALL) NOPASSWD: /usr/bin/chown mysql\:mysql /run/mysqld
xamgibson ALL=(ALL) NOPASSWD: /usr/sbin/service mariadb start
xamgibson ALL=(ALL) NOPASSWD: /usr/sbin/service mariadb stop
xamgibson ALL=(ALL) NOPASSWD: /usr/sbin/service mariadb restart
xamgibson ALL=(ALL) NOPASSWD: /usr/sbin/service mysql start
xamgibson ALL=(ALL) NOPASSWD: /usr/sbin/service mysql stop
xamgibson ALL=(ALL) NOPASSWD: /usr/sbin/service mysql restart
EOF

# Validate the sudoers file
if visudo -c -f /tmp/mariadb-sudoers; then
    # Install it to sudoers.d
    cp /tmp/mariadb-sudoers /etc/sudoers.d/mariadb-startup
    chmod 440 /etc/sudoers.d/mariadb-startup
    echo "Sudoers file installed successfully!"
    echo "MariaDB can now be started without password prompts."
else
    echo "Error in sudoers file syntax. Not installing."
    exit 1
fi

rm /tmp/mariadb-sudoers