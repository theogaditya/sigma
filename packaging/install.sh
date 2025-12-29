#!/bin/bash
#
# Sigma Language Installer
# One-line install: curl -sSL https://raw.githubusercontent.com/YOUR_USERNAME/sigma-lang/main/packaging/install.sh | bash
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Configuration
REPO="YOUR_USERNAME/sigma-lang"  # TODO: Update with actual repo
INSTALL_DIR="/usr/local/bin"
BINARY_NAME="sigma"

echo -e "${CYAN}${BOLD}"
echo "  ╔═══════════════════════════════════════╗"
echo "  ║     Sigma Language Installer  🔥      ║"
echo "  ╚═══════════════════════════════════════╝"
echo -e "${NC}"

# Detect OS and architecture
detect_platform() {
    OS="$(uname -s)"
    ARCH="$(uname -m)"

    case "$OS" in
        Linux*)     PLATFORM="linux" ;;
        Darwin*)    PLATFORM="macos" ;;
        MINGW*|MSYS*|CYGWIN*)
            echo -e "${RED}Error: Windows detected. Please use install.ps1 instead.${NC}"
            exit 1
            ;;
        *)
            echo -e "${RED}Error: Unsupported operating system: $OS${NC}"
            exit 1
            ;;
    esac

    case "$ARCH" in
        x86_64|amd64)   ARCH="x86_64" ;;
        aarch64|arm64)  ARCH="arm64" ;;
        *)
            echo -e "${RED}Error: Unsupported architecture: $ARCH${NC}"
            exit 1
            ;;
    esac

    BINARY="sigma-${PLATFORM}-${ARCH}"
    echo -e "${GREEN}✓${NC} Detected platform: ${BOLD}${PLATFORM}-${ARCH}${NC}"
}

# Get latest release version
get_latest_version() {
    echo -e "${CYAN}→${NC} Fetching latest version..."
    
    if command -v curl &> /dev/null; then
        VERSION=$(curl -sL "https://api.github.com/repos/${REPO}/releases/latest" | grep '"tag_name"' | sed -E 's/.*"([^"]+)".*/\1/')
    elif command -v wget &> /dev/null; then
        VERSION=$(wget -qO- "https://api.github.com/repos/${REPO}/releases/latest" | grep '"tag_name"' | sed -E 's/.*"([^"]+)".*/\1/')
    else
        echo -e "${RED}Error: Neither curl nor wget found. Please install one of them.${NC}"
        exit 1
    fi

    if [ -z "$VERSION" ]; then
        echo -e "${YELLOW}Warning: Could not fetch latest version. Using 'latest'.${NC}"
        VERSION="latest"
    fi
    
    echo -e "${GREEN}✓${NC} Latest version: ${BOLD}${VERSION}${NC}"
}

# Download binary
download_binary() {
    echo -e "${CYAN}→${NC} Downloading ${BINARY}..."
    
    DOWNLOAD_URL="https://github.com/${REPO}/releases/download/${VERSION}/${BINARY}"
    TMP_DIR=$(mktemp -d)
    TMP_FILE="${TMP_DIR}/${BINARY_NAME}"
    
    if command -v curl &> /dev/null; then
        curl -sL "$DOWNLOAD_URL" -o "$TMP_FILE"
    else
        wget -q "$DOWNLOAD_URL" -O "$TMP_FILE"
    fi

    if [ ! -f "$TMP_FILE" ] || [ ! -s "$TMP_FILE" ]; then
        echo -e "${RED}Error: Failed to download binary.${NC}"
        echo -e "${YELLOW}URL: ${DOWNLOAD_URL}${NC}"
        rm -rf "$TMP_DIR"
        exit 1
    fi

    chmod +x "$TMP_FILE"
    echo -e "${GREEN}✓${NC} Downloaded successfully"
}

# Install binary
install_binary() {
    echo -e "${CYAN}→${NC} Installing to ${INSTALL_DIR}..."
    
    # Check if we need sudo
    if [ -w "$INSTALL_DIR" ]; then
        mv "$TMP_FILE" "${INSTALL_DIR}/${BINARY_NAME}"
    else
        echo -e "${YELLOW}  Need sudo to install to ${INSTALL_DIR}${NC}"
        sudo mv "$TMP_FILE" "${INSTALL_DIR}/${BINARY_NAME}"
        sudo chmod +x "${INSTALL_DIR}/${BINARY_NAME}"
    fi

    rm -rf "$TMP_DIR"
    echo -e "${GREEN}✓${NC} Installed to ${INSTALL_DIR}/${BINARY_NAME}"
}

# Verify installation
verify_install() {
    if command -v sigma &> /dev/null; then
        echo ""
        echo -e "${GREEN}${BOLD}✓ Sigma installed successfully!${NC}"
        echo ""
        sigma --version
        echo ""
        echo -e "${CYAN}Get started:${NC}"
        echo "  sigma --help          # Show help"
        echo "  sigma program.sigma   # Run a program"
        echo "  sigma                 # Start REPL"
        echo ""
    else
        echo ""
        echo -e "${YELLOW}Warning: 'sigma' command not found in PATH.${NC}"
        echo -e "You may need to add ${INSTALL_DIR} to your PATH:"
        echo ""
        echo "  export PATH=\"\$PATH:${INSTALL_DIR}\""
        echo ""
    fi
}

# Main installation flow
main() {
    detect_platform
    get_latest_version
    download_binary
    install_binary
    verify_install
}

main
