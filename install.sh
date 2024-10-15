#!/bin/bash

# Install script for the Todo CLI application

set -e  # Exit immediately if a command exits with a non-zero status

# Function to display error messages
error() {
    echo "Error: $1" >&2
    exit 1
}

# Function to check if a package is installed on Debian-based systems
is_debian_package_installed() {
    dpkg -s "$1" &> /dev/null
}

# Function to check if a package is installed on Red Hat-based systems
is_redhat_package_installed() {
    rpm -q "$1" &> /dev/null
}

# Function to check if a package is installed on Arch-based systems
is_arch_package_installed() {
    pacman -Qi "$1" &> /dev/null
}

# Determine the operating system
OS=""
PACKAGE_MANAGER=""
if [ "$(uname)" == "Darwin" ]; then
    OS="macOS"
    PACKAGE_MANAGER="brew"
elif [ -f "/etc/debian_version" ]; then
    OS="Debian"
    PACKAGE_MANAGER="apt-get"
elif [ -f "/etc/redhat-release" ]; then
    OS="RedHat"
    PACKAGE_MANAGER="dnf"
elif [ -f "/etc/arch-release" ]; then
    OS="Arch"
    PACKAGE_MANAGER="pacman"
else
    error "Unsupported operating system. Please install manually."
fi

echo "Operating System detected: $OS"

# Define dependencies based on OS
DEPENDENCIES_GLOBAL=("gcc" "make" "libncurses5-dev" "libc6-dev")
DEPENDENCIES_MAC=("gcc" "make" "ncurses")

# Function to install dependencies on Debian-based systems
install_debian_dependencies() {
    sudo apt-get update
    for pkg in "${DEPENDENCIES_GLOBAL[@]}"; do
        if ! is_debian_package_installed "$pkg"; then
            sudo apt-get install -y "$pkg"
        fi
    done
}

# Function to install dependencies on Red Hat-based systems
install_redhat_dependencies() {
    sudo dnf install -y gcc make ncurses-devel glibc-devel
}

# Function to install dependencies on Arch-based systems
install_arch_dependencies() {
    sudo pacman -Sy --noconfirm base-devel ncurses
}

# Function to install dependencies on macOS using Homebrew
install_macos_dependencies() {
    # Check if Homebrew is installed
    if ! command -v brew &> /dev/null; then
        echo "Homebrew is not installed. Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
        echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.bash_profile
        eval "$(/opt/homebrew/bin/brew shellenv)"
    fi
    brew update
    for pkg in "${DEPENDENCIES_MAC[@]}"; do
        if ! brew list "$pkg" &> /dev/null; then
            brew install "$pkg"
        fi
    done
}

# Check for dependencies based on OS
echo "Checking for dependencies..."

MISSING_DEPS=()

if [ "$OS" == "Debian" ]; then
    for pkg in "${DEPENDENCIES_GLOBAL[@]}"; do
        if ! is_debian_package_installed "$pkg"; then
            MISSING_DEPS+=("$pkg")
        fi
    done
elif [ "$OS" == "RedHat" ]; then
    # RedHat uses different package names for development libraries
    REDHAT_DEPENDENCIES=("gcc" "make" "ncurses-devel" "glibc-devel")
    for pkg in "${REDHAT_DEPENDENCIES[@]}"; do
        if ! is_redhat_package_installed "$pkg"; then
            MISSING_DEPS+=("$pkg")
        fi
    done
elif [ "$OS" == "Arch" ]; then
    for pkg in "${DEPENDENCIES_GLOBAL[@]:0:3}"; do  # Arch uses 'base-devel' and 'ncurses'
        if ! is_arch_package_installed "$pkg"; then
            MISSING_DEPS+=("$pkg")
        fi
    done
elif [ "$OS" == "macOS" ]; then
    for pkg in "${DEPENDENCIES_MAC[@]}"; do
        if ! command -v "$pkg" &> /dev/null; then
            MISSING_DEPS+=("$pkg")
        fi
    done
fi

if [ ${#MISSING_DEPS[@]} -ne 0 ]; then
    echo "The following dependencies are missing: ${MISSING_DEPS[@]}"
    read -p "Do you want to install them? [Y/n]: " INSTALL_DEPS
    INSTALL_DEPS=${INSTALL_DEPS:-Y}
    if [[ "$INSTALL_DEPS" =~ ^[Yy]$ ]]; then
        # Install dependencies based on the OS
        if [ "$OS" == "Debian" ]; then
            install_debian_dependencies
        elif [ "$OS" == "RedHat" ]; then
            install_redhat_dependencies
        elif [ "$OS" == "Arch" ]; then
            install_arch_dependencies
        elif [ "$OS" == "macOS" ]; then
            install_macos_dependencies
        fi
    else
        error "Dependencies are required to proceed with the installation."
    fi
else
    echo "All dependencies are satisfied."
fi

# Ask the user for installation type
echo "Select installation type:"
echo "1) Global Install (requires sudo)"
echo "2) Local Install (user-specific)"
read -p "Enter your choice [1/2]: " INSTALL_TYPE

# Validate user input
if [ "$INSTALL_TYPE" == "1" ]; then
    INSTALL_CHOICE="Global"
elif [ "$INSTALL_TYPE" == "2" ]; then
    INSTALL_CHOICE="Local"
else
    error "Invalid choice. Please run the script again and select 1 or 2."
fi

echo "You have chosen: $INSTALL_CHOICE Install"

# Build the application
echo "Building the application..."
cd build || error "Build directory not found."
make clean
make

# Ensure the data directory is set up
DATA_DIR="$HOME/.local/share/todo"
if [ ! -d "$DATA_DIR" ]; then
    echo "Creating data directory at $DATA_DIR"
    mkdir -p "$DATA_DIR"
fi

# Proceed with the chosen installation type
if [ "$INSTALL_CHOICE" == "Global" ]; then
    echo "Installing globally..."
    sudo make install
    if [ $? -eq 0 ]; then
        echo "Global installation complete."
        echo "You can run the application using the command 'todo'"
    else
        error "Global installation failed."
    fi
else
    echo "Installing locally..."
    LOCAL_BIN_DIR="$HOME/.local/bin"
    mkdir -p "$LOCAL_BIN_DIR"
    cp binary/todo "$LOCAL_BIN_DIR/todo"
    chmod +x "$LOCAL_BIN_DIR/todo"
    echo "Local installation complete."
    echo "You can run the application using the command '$LOCAL_BIN_DIR/todo'"
    echo "For convenience, you can add '$LOCAL_BIN_DIR' to your PATH:"
    echo "    export PATH=\"\$PATH:$LOCAL_BIN_DIR\""
    echo "You can add the above line to your shell configuration file (e.g., ~/.bashrc or ~/.zshrc)."
fi

echo "Installation finished successfully."
