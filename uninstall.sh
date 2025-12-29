#!/bin/bash
#
# Sigma Language Uninstaller
# Removes the sigma compiler from your system
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Installation locations to check
SYSTEM_INSTALL="/usr/local/bin/sigma"
USER_INSTALL="$HOME/.local/bin/sigma"

echo -e "${BLUE}"
echo "  _____ _                       "
echo " / ____(_)                      "
echo "| (___  _  __ _ _ __ ___   __ _ "
echo " \___ \| |/ _\` | '_ \` _ \ / _\` |"
echo " ____) | | (_| | | | | | | (_| |"
echo "|_____/|_|\__, |_| |_| |_|\__,_|"
echo "           __/ |                "
echo "          |___/                 "
echo -e "${NC}"
echo -e "${RED}Sigma Language Uninstaller${NC}"
echo "================================"
echo ""

# Find and remove sigma
uninstall() {
    local found=false
    
    # Check system installation
    if [ -f "$SYSTEM_INSTALL" ]; then
        echo -e "Found sigma at: ${YELLOW}$SYSTEM_INSTALL${NC}"
        read -p "Remove? [y/N]: " confirm
        if [[ "$confirm" =~ ^[Yy]$ ]]; then
            sudo rm -f "$SYSTEM_INSTALL"
            echo -e "${GREEN}✓${NC} Removed $SYSTEM_INSTALL"
            found=true
        fi
    fi
    
    # Check user installation
    if [ -f "$USER_INSTALL" ]; then
        echo -e "Found sigma at: ${YELLOW}$USER_INSTALL${NC}"
        read -p "Remove? [y/N]: " confirm
        if [[ "$confirm" =~ ^[Yy]$ ]]; then
            rm -f "$USER_INSTALL"
            echo -e "${GREEN}✓${NC} Removed $USER_INSTALL"
            found=true
        fi
    fi
    
    # Check if sigma is in PATH somewhere else
    local sigma_path=$(command -v sigma 2>/dev/null || true)
    if [ -n "$sigma_path" ] && [ "$sigma_path" != "$SYSTEM_INSTALL" ] && [ "$sigma_path" != "$USER_INSTALL" ]; then
        echo -e "Found sigma at: ${YELLOW}$sigma_path${NC}"
        read -p "Remove? [y/N]: " confirm
        if [[ "$confirm" =~ ^[Yy]$ ]]; then
            if [[ "$sigma_path" == /usr/* ]]; then
                sudo rm -f "$sigma_path"
            else
                rm -f "$sigma_path"
            fi
            echo -e "${GREEN}✓${NC} Removed $sigma_path"
            found=true
        fi
    fi
    
    if [ "$found" = false ]; then
        echo -e "${YELLOW}No Sigma installation found.${NC}"
        echo ""
        echo "Checked locations:"
        echo "  - $SYSTEM_INSTALL"
        echo "  - $USER_INSTALL"
        exit 0
    fi
}

# Clean build directory
clean_build() {
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    BUILD_DIR="$SCRIPT_DIR/build"
    
    if [ -d "$BUILD_DIR" ]; then
        echo ""
        read -p "Also remove build directory ($BUILD_DIR)? [y/N]: " confirm
        if [[ "$confirm" =~ ^[Yy]$ ]]; then
            rm -rf "$BUILD_DIR"
            echo -e "${GREEN}✓${NC} Removed build directory"
        fi
    fi
}

# Main
main() {
    uninstall
    clean_build
    
    echo ""
    echo "================================"
    echo -e "${GREEN}✅ Uninstallation complete!${NC}"
    echo ""
    echo -e "${BLUE}Sad to see you go... stay sigma! 😢${NC}"
}

main
