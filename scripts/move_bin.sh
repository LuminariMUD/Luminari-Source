#!/bin/bash

# move_bin.sh - Consolidated binary deployment script for MUD campaigns
# Supports Forgotten Realms (fr), Dragonlance (dl), and Luminari campaigns
# Handles both dev and live environment deployments

set -e  # Exit immediately if a command exits with a non-zero status

# Function to display usage information
usage() {
    echo "Usage: $0 --campaign <campaign> --env <environment>"
    echo ""
    echo "Campaigns:"
    echo "  fr        Forgotten Realms"
    echo "  dl        Dragonlance" 
    echo "  luminari  Luminari (default campaign)"
    echo ""
    echo "Environments:"
    echo "  dev       Development environment (auto-starts server)"
    echo "  live      Live/Production environment (manual restart required)"
    echo ""
    echo "Examples:"
    echo "  $0 --campaign fr --env dev"
    echo "  $0 --campaign dl --env live"
    echo "  $0 --campaign luminari --env dev"
    echo ""
    exit 1
}

# Initialize variables
CAMPAIGN=""
ENV=""

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --campaign)
            CAMPAIGN="$2"
            shift 2
            ;;
        --env)
            ENV="$2"
            shift 2
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo "Error: Unknown option $1"
            usage
            ;;
    esac
done

# Validate required arguments
if [[ -z "$CAMPAIGN" || -z "$ENV" ]]; then
    echo "Error: Both --campaign and --env are required"
    usage
fi

# Validate campaign options
case $CAMPAIGN in
    fr|dl|luminari)
        ;;
    *)
        echo "Error: Invalid campaign '$CAMPAIGN'. Must be: fr, dl, or luminari"
        usage
        ;;
esac

# Validate environment options
case $ENV in
    dev|live)
        ;;
    *)
        echo "Error: Invalid environment '$ENV'. Must be: dev or live"
        usage
        ;;
esac

# Set up paths and descriptions based on campaign and environment
case $CAMPAIGN in
    fr)
        if [[ "$ENV" == "dev" ]]; then
            BASE_PATH="/home/frmud/dev"
            DESCRIPTION="frmud dev port"
        else
            BASE_PATH="/home/frmud/frmud"
            DESCRIPTION="frmud live port"
        fi
        ;;
    dl)
        if [[ "$ENV" == "dev" ]]; then
            BASE_PATH="/home/krynn/dev"
            DESCRIPTION="aod reawakening dev port"
        else
            BASE_PATH="/home/krynn/live"
            DESCRIPTION="aod reawakening live port"
        fi
        ;;
    luminari)
        if [[ "$ENV" == "dev" ]]; then
            BASE_PATH="/home/luminari/dev"
            DESCRIPTION="luminari dev port"
        else
            BASE_PATH="/home/luminari/mud"
            DESCRIPTION="luminari live port"
        fi
        ;;
esac

# Check if source binary exists
if [[ ! -f "bin/circle" ]]; then
    echo "Error: Source binary 'bin/circle' not found!"
    exit 1
fi

# Check if target directory exists
if [[ ! -d "$BASE_PATH" ]]; then
    echo "Error: Target directory '$BASE_PATH' not found!"
    exit 1
fi

echo "=================================================="
echo "MUD Binary Deployment"
echo "Campaign: $CAMPAIGN"
echo "Environment: $ENV"
echo "Target: $BASE_PATH"
echo "=================================================="

# Create timestamped backup of existing binary
TIMEDATE=$(date +"%m-%d-%Y-%H-%M")
if [[ -f "$BASE_PATH/bin/circle" ]]; then
    echo "Creating backup: circle.$TIMEDATE"
    mv "$BASE_PATH/bin/circle" "$BASE_PATH/bin/circle.$TIMEDATE"
else
    echo "No existing binary found, skipping backup"
fi

# Copy new binary
echo "Copying new binary..."
cp "bin/circle" "$BASE_PATH/bin/"
echo "Copied binary to $DESCRIPTION"

# Copy changelog to news file
if [[ -f "$BASE_PATH/changelog" ]]; then
    cp "$BASE_PATH/changelog" "$BASE_PATH/lib/text/news"
    echo "Moved changelog over to news file"
else
    echo "Warning: No changelog found at $BASE_PATH/changelog"
fi

echo ""

# Environment-specific actions
if [[ "$ENV" == "dev" ]]; then
    # Development environment - auto-start server
    echo "Development environment detected"
    echo "1 second delay before starting server..."
    sleep 1
    
    if [[ "$CAMPAIGN" == "luminari" ]]; then
        echo "Starting dev server on port 4101"
    else
        echo "Starting dev server"
    fi
    
    cd "$BASE_PATH" && ./checkmud.sh &
    echo ""
    echo "Server started in background"
    echo "To change to the dev directory, run:"
    echo "cd $BASE_PATH"
    
else
    # Live environment - manual restart required
    echo "Binary deployment complete for LIVE server."
    echo "To change to the live directory, run:"
    echo "cd $BASE_PATH"
    echo ""
    echo "WARNING: Remember to manually restart the live server when ready."
fi

echo ""
echo "Deployment completed successfully!"