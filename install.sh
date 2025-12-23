#!/bin/bash
#
# Sigma Language Installer
# Installs the sigma compiler to your system
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Installation directory options
SYSTEM_INSTALL="/usr/local/bin"
USER_INSTALL="$HOME/.local/bin"

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
echo -e "${GREEN}Sigma Language Installer${NC}"
echo "================================"
echo ""

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
BINARY="$BUILD_DIR/bin/sigma"

# Check for required tools
check_requirements() {
    echo -e "${BLUE}[1/4]${NC} Checking requirements..."
    
    local missing=()
    
    if ! command -v cmake &> /dev/null; then
        missing+=("cmake")
    fi
    
    if ! command -v make &> /dev/null; then
        missing+=("make")
    fi
    
    if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        missing+=("g++ or clang++")
    fi
    
    if [ ${#missing[@]} -ne 0 ]; then
        echo -e "${RED}Error: Missing required tools: ${missing[*]}${NC}"
        echo "Please install them and try again."
        exit 1
    fi
    
    echo -e "  ${GREEN}✓${NC} All requirements satisfied"
}

# Build the compiler
build_compiler() {
    echo -e "${BLUE}[2/4]${NC} Building Sigma compiler..."
    
    # Create build directory
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Run cmake
    echo "  Running cmake..."
    cmake .. > /dev/null 2>&1 || {
        echo -e "${RED}Error: cmake configuration failed${NC}"
        cmake ..
        exit 1
    }
    
    # Build
    echo "  Compiling..."
    make -j$(nproc 2>/dev/null || echo 2) > /dev/null 2>&1 || {
        echo -e "${RED}Error: Build failed${NC}"
        make
        exit 1
    }
    
    cd "$SCRIPT_DIR"
    
    if [ -f "$BINARY" ]; then
        echo -e "  ${GREEN}✓${NC} Build successful"
    else
        echo -e "${RED}Error: Binary not found after build${NC}"
        exit 1
    fi
}

# Install the binary
install_binary() {
    echo -e "${BLUE}[3/4]${NC} Installing Sigma..."
    
    # Ask user for installation type
    echo ""
    echo "Where would you like to install Sigma?"
    echo "  1) System-wide ($SYSTEM_INSTALL) - requires sudo"
    echo "  2) User only ($USER_INSTALL) - no sudo required"
    echo ""
    read -p "Enter choice [1/2] (default: 2): " choice
    
    case "$choice" in
        1)
            INSTALL_DIR="$SYSTEM_INSTALL"
            USE_SUDO=true
            ;;
        *)
            INSTALL_DIR="$USER_INSTALL"
            USE_SUDO=false
            # Create user bin directory if it doesn't exist
            mkdir -p "$INSTALL_DIR"
            ;;
    esac
    
    # Copy binary
    if [ "$USE_SUDO" = true ]; then
        echo "  Installing to $INSTALL_DIR (requires sudo)..."
        sudo cp "$BINARY" "$INSTALL_DIR/sigma"
        sudo chmod +x "$INSTALL_DIR/sigma"
    else
        echo "  Installing to $INSTALL_DIR..."
        cp "$BINARY" "$INSTALL_DIR/sigma"
        chmod +x "$INSTALL_DIR/sigma"
    fi
    
    echo -e "  ${GREEN}✓${NC} Installed to $INSTALL_DIR/sigma"
}

# Setup PATH if needed
setup_path() {
    echo -e "${BLUE}[4/4]${NC} Checking PATH..."
    
    if [[ ":$PATH:" == *":$INSTALL_DIR:"* ]]; then
        echo -e "  ${GREEN}✓${NC} $INSTALL_DIR is already in PATH"
    else
        echo -e "  ${YELLOW}!${NC} $INSTALL_DIR is not in your PATH"
        echo ""
        echo "  Add this line to your ~/.bashrc or ~/.zshrc:"
        echo -e "  ${YELLOW}export PATH=\"$INSTALL_DIR:\$PATH\"${NC}"
        echo ""
        
        read -p "  Would you like to add it automatically? [y/N]: " add_path
        
        if [[ "$add_path" =~ ^[Yy]$ ]]; then
            # Detect shell config file
            if [ -f "$HOME/.zshrc" ]; then
                SHELL_RC="$HOME/.zshrc"
            else
                SHELL_RC="$HOME/.bashrc"
            fi
            
            echo "" >> "$SHELL_RC"
            echo "# Sigma Language" >> "$SHELL_RC"
            echo "export PATH=\"$INSTALL_DIR:\$PATH\"" >> "$SHELL_RC"
            
            echo -e "  ${GREEN}✓${NC} Added to $SHELL_RC"
            echo -e "  ${YELLOW}!${NC} Run 'source $SHELL_RC' or restart your terminal"
        fi
    fi
}

# Verify installation
verify_installation() {
    echo ""
    echo "================================"
    
    if command -v sigma &> /dev/null; then
        echo -e "${GREEN}✅ Installation complete!${NC}"
        echo ""
        echo "Try these commands:"
        echo -e "  ${BLUE}sigma --help${NC}          Show help"
        echo -e "  ${BLUE}sigma --version${NC}       Show version"
        echo -e "  ${BLUE}sigma program.sigma${NC}   Compile a program"
        echo -e "  ${BLUE}sigma${NC}                 Start REPL"
    else
        echo -e "${GREEN}✅ Installation complete!${NC}"
        echo ""
        echo -e "${YELLOW}Note: You may need to restart your terminal or run:${NC}"
        echo -e "  source ~/.bashrc  (or ~/.zshrc)"
        echo ""
        echo "Then try:"
        echo -e "  ${BLUE}sigma --help${NC}"
    fi
    
    echo ""
    echo -e "${BLUE}Stay sigma! 🔥${NC}"
}

# Main installation flow
main() {
    check_requirements
    build_compiler
    install_binary
    setup_path
    verify_installation
}

# Run main
main
