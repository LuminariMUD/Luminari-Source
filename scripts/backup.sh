#!/bin/bash
#
# LuminariMUD Comprehensive Backup Script
# Backs up database, world files, and player data
#
# Usage: ./scripts/backup.sh [options]
#   -d, --db-only       Backup database only
#   -w, --world-only    Backup world files only
#   -p, --player-only   Backup player data only
#   -f, --full          Full backup (default)
#   -r, --retention N   Keep N days of backups (default: 7)
#   -o, --output DIR    Backup destination directory
#   -v, --verbose       Verbose output
#   -h, --help          Show help
#
# Cron example (daily at 2 AM):
#   0 2 * * * /path/to/Luminari-Source/scripts/backup.sh >> /var/log/luminari-backup.log 2>&1
#

set -euo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TIMESTAMP=$(date +%Y%m%d-%H%M%S)
DATE_ONLY=$(date +%Y%m%d)

# Default settings
BACKUP_DIR="${PROJECT_ROOT}/backups"
RETENTION_DAYS=7
VERBOSE=false
BACKUP_DB=true
BACKUP_WORLD=true
BACKUP_PLAYERS=true

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Logging
log() {
    local level="$1"
    shift
    local msg="[$(date '+%Y-%m-%d %H:%M:%S')] [$level] $*"
    if [[ "$VERBOSE" == true ]] || [[ "$level" != "DEBUG" ]]; then
        echo -e "$msg"
    fi
}

log_info() { log "INFO" "$@"; }
log_warn() { log "WARN" "${YELLOW}$*${NC}"; }
log_error() { log "ERROR" "${RED}$*${NC}"; }
log_success() { log "OK" "${GREEN}$*${NC}"; }
log_debug() { log "DEBUG" "$@"; }

# Help message
show_help() {
    cat <<EOF
LuminariMUD Backup Script

Usage: $0 [options]

Options:
    -d, --db-only       Backup database only
    -w, --world-only    Backup world files only
    -p, --player-only   Backup player data only
    -f, --full          Full backup (default)
    -r, --retention N   Keep N days of backups (default: 7)
    -o, --output DIR    Backup destination directory
    -v, --verbose       Verbose output
    -h, --help          Show this help

Cron example (daily at 2 AM):
    0 2 * * * $PROJECT_ROOT/scripts/backup.sh >> /var/log/luminari-backup.log 2>&1

Backup contents:
    - Database: MySQL/MariaDB dump of luminari database
    - World: All zone files (zon, wld, mob, obj, shp, trg, qst, hlq)
    - Players: Player files and objects (plrfiles, plrobjs)
    - Config: lib/etc and lib/misc directories
EOF
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--db-only)
            BACKUP_DB=true
            BACKUP_WORLD=false
            BACKUP_PLAYERS=false
            shift
            ;;
        -w|--world-only)
            BACKUP_DB=false
            BACKUP_WORLD=true
            BACKUP_PLAYERS=false
            shift
            ;;
        -p|--player-only)
            BACKUP_DB=false
            BACKUP_WORLD=false
            BACKUP_PLAYERS=true
            shift
            ;;
        -f|--full)
            BACKUP_DB=true
            BACKUP_WORLD=true
            BACKUP_PLAYERS=true
            shift
            ;;
        -r|--retention)
            RETENTION_DAYS="$2"
            shift 2
            ;;
        -o|--output)
            BACKUP_DIR="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Create backup directory
mkdir -p "$BACKUP_DIR"
CURRENT_BACKUP="${BACKUP_DIR}/${DATE_ONLY}"
mkdir -p "$CURRENT_BACKUP"

log_info "Starting LuminariMUD backup to $CURRENT_BACKUP"
log_info "Retention policy: $RETENTION_DAYS days"

# Track backup status
BACKUP_SUCCESS=true
BACKUP_COMPONENTS=()

# Backup database
backup_database() {
    log_info "Backing up database..."

    local mysql_config="${PROJECT_ROOT}/lib/mysql_config"
    local db_backup="${CURRENT_BACKUP}/database-${TIMESTAMP}.sql.gz"

    if [[ ! -f "$mysql_config" ]]; then
        log_warn "MySQL config not found at $mysql_config"
        log_warn "Database backup skipped - configure lib/mysql_config first"
        return 1
    fi

    # Parse MySQL config
    local db_host=$(grep "mysql_host" "$mysql_config" | cut -d'=' -f2 | tr -d ' ')
    local db_name=$(grep "mysql_database" "$mysql_config" | cut -d'=' -f2 | tr -d ' ')
    local db_user=$(grep "mysql_username" "$mysql_config" | cut -d'=' -f2 | tr -d ' ')
    local db_pass=$(grep "mysql_password" "$mysql_config" | cut -d'=' -f2 | tr -d ' ')

    if [[ -z "$db_name" ]] || [[ -z "$db_user" ]]; then
        log_error "Invalid MySQL configuration"
        return 1
    fi

    log_debug "Database: $db_name on $db_host"

    # Perform backup
    if command -v mysqldump >/dev/null 2>&1; then
        if mysqldump -h "$db_host" -u "$db_user" -p"$db_pass" \
            --single-transaction --quick --lock-tables=false \
            "$db_name" 2>/dev/null | gzip > "$db_backup"; then
            local size=$(du -h "$db_backup" | cut -f1)
            log_success "Database backup complete: $db_backup ($size)"
            BACKUP_COMPONENTS+=("database")
            return 0
        else
            log_error "Database backup failed"
            rm -f "$db_backup"
            return 1
        fi
    else
        log_error "mysqldump not found - cannot backup database"
        return 1
    fi
}

# Backup world files
backup_world() {
    log_info "Backing up world files..."

    local world_dir="${PROJECT_ROOT}/lib/world"
    local world_backup="${CURRENT_BACKUP}/world-${TIMESTAMP}.tar.gz"

    if [[ ! -d "$world_dir" ]]; then
        log_error "World directory not found: $world_dir"
        return 1
    fi

    # Create tarball of all world data
    if tar -czf "$world_backup" -C "${PROJECT_ROOT}/lib" \
        world/zon world/wld world/mob world/obj \
        world/shp world/trg world/qst world/hlq 2>/dev/null; then
        local size=$(du -h "$world_backup" | cut -f1)
        log_success "World backup complete: $world_backup ($size)"
        BACKUP_COMPONENTS+=("world")
        return 0
    else
        log_error "World backup failed"
        rm -f "$world_backup"
        return 1
    fi
}

# Backup player data
backup_players() {
    log_info "Backing up player data..."

    local lib_dir="${PROJECT_ROOT}/lib"
    local player_backup="${CURRENT_BACKUP}/players-${TIMESTAMP}.tar.gz"

    # Check for player directories
    local dirs_to_backup=()
    for dir in plrfiles plrobjs house mudmail; do
        if [[ -d "${lib_dir}/${dir}" ]]; then
            dirs_to_backup+=("$dir")
        fi
    done

    if [[ ${#dirs_to_backup[@]} -eq 0 ]]; then
        log_warn "No player data directories found"
        return 1
    fi

    log_debug "Backing up: ${dirs_to_backup[*]}"

    # Create tarball of player data
    if tar -czf "$player_backup" -C "$lib_dir" "${dirs_to_backup[@]}" 2>/dev/null; then
        local size=$(du -h "$player_backup" | cut -f1)
        log_success "Player backup complete: $player_backup ($size)"
        BACKUP_COMPONENTS+=("players")
        return 0
    else
        log_error "Player backup failed"
        rm -f "$player_backup"
        return 1
    fi
}

# Backup configuration
backup_config() {
    log_info "Backing up configuration..."

    local lib_dir="${PROJECT_ROOT}/lib"
    local config_backup="${CURRENT_BACKUP}/config-${TIMESTAMP}.tar.gz"

    # Config directories to backup
    local dirs_to_backup=()
    for dir in etc misc text; do
        if [[ -d "${lib_dir}/${dir}" ]]; then
            dirs_to_backup+=("$dir")
        fi
    done

    if [[ ${#dirs_to_backup[@]} -eq 0 ]]; then
        log_warn "No configuration directories found"
        return 1
    fi

    # Create tarball of config
    if tar -czf "$config_backup" -C "$lib_dir" "${dirs_to_backup[@]}" 2>/dev/null; then
        local size=$(du -h "$config_backup" | cut -f1)
        log_success "Config backup complete: $config_backup ($size)"
        BACKUP_COMPONENTS+=("config")
        return 0
    else
        log_error "Config backup failed"
        rm -f "$config_backup"
        return 1
    fi
}

# Clean up old backups
cleanup_old_backups() {
    log_info "Cleaning up backups older than $RETENTION_DAYS days..."

    if [[ "$RETENTION_DAYS" -le 0 ]]; then
        log_debug "Retention disabled, skipping cleanup"
        return 0
    fi

    local count=0
    while IFS= read -r -d '' dir; do
        log_debug "Removing old backup: $dir"
        rm -rf "$dir"
        count=$((count + 1))
    done < <(find "$BACKUP_DIR" -maxdepth 1 -type d -mtime +$RETENTION_DAYS -print0 2>/dev/null)

    if [[ $count -gt 0 ]]; then
        log_info "Removed $count old backup(s)"
    else
        log_debug "No old backups to remove"
    fi
}

# Create backup manifest
create_manifest() {
    local manifest="${CURRENT_BACKUP}/MANIFEST.txt"

    cat > "$manifest" <<EOF
LuminariMUD Backup Manifest
===========================
Date: $(date)
Hostname: $(hostname)
Backup Directory: $CURRENT_BACKUP
Retention Policy: $RETENTION_DAYS days

Components Backed Up:
EOF

    for component in "${BACKUP_COMPONENTS[@]}"; do
        echo "  - $component" >> "$manifest"
    done

    echo "" >> "$manifest"
    echo "Files:" >> "$manifest"
    ls -lh "$CURRENT_BACKUP" | tail -n +2 >> "$manifest"

    log_debug "Manifest created: $manifest"
}

# Main execution
main() {
    local start_time=$(date +%s)

    # Perform backups
    if [[ "$BACKUP_DB" == true ]]; then
        backup_database || BACKUP_SUCCESS=false
    fi

    if [[ "$BACKUP_WORLD" == true ]]; then
        backup_world || BACKUP_SUCCESS=false
    fi

    if [[ "$BACKUP_PLAYERS" == true ]]; then
        backup_players || BACKUP_SUCCESS=false
        backup_config || true  # Config is optional
    fi

    # Cleanup old backups
    cleanup_old_backups

    # Create manifest
    create_manifest

    # Summary
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    local total_size=$(du -sh "$CURRENT_BACKUP" | cut -f1)

    echo ""
    log_info "=========================================="
    log_info "Backup Summary"
    log_info "=========================================="
    log_info "Location: $CURRENT_BACKUP"
    log_info "Total Size: $total_size"
    log_info "Duration: ${duration}s"
    log_info "Components: ${BACKUP_COMPONENTS[*]:-none}"

    if [[ "$BACKUP_SUCCESS" == true ]]; then
        log_success "Backup completed successfully!"
        exit 0
    else
        log_warn "Backup completed with warnings"
        exit 1
    fi
}

# Run main
main
