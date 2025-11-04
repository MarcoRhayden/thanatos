#!/bin/bash

# Detect Linux distribution
detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        echo $ID
    elif [ -f /etc/redhat-release ]; then
        echo "rhel"
    elif [ -f /etc/debian_version ]; then
        echo "debian"
    else
        echo "unknown"
    fi
}

install_packages() {
    local distro=$(detect_distro)
    
    echo "Detected distribution: $distro"
    
    case $distro in
        "arch"|"manjaro")
            echo "Installing packages with pacman..."
            sudo pacman -Syu --needed base-devel git curl zip unzip tar cmake ninja
            ;;
        "ubuntu"|"debian"|"pop"|"linuxmint")
            echo "Installing packages with apt..."
            sudo apt update
            sudo apt install -y build-essential git curl zip unzip tar cmake ninja-build pkg-config
            ;;
        "fedora"|"rhel"|"centos"|"rocky"|"almalinux")
            echo "Installing packages with dnf/yum..."
            if command -v dnf &> /dev/null; then
                sudo dnf groupinstall -y "Development Tools"
                sudo dnf install -y git curl zip unzip tar cmake ninja-build pkg-config
            else
                sudo yum groupinstall -y "Development Tools"
                sudo yum install -y git curl zip unzip tar cmake ninja-build pkg-config
            fi
            ;;
        "opensuse"|"sles")
            echo "Installing packages with zypper..."
            sudo zypper install -y -t pattern devel_basis
            sudo zypper install -y git curl zip unzip tar cmake ninja pkg-config
            ;;
        *)
            echo "Unsupported distribution: $distro"
            echo "Please install the following packages manually:"
            echo "- build tools (gcc, g++, make)"
            echo "- git, curl, zip, unzip, tar"
            echo "- cmake (>= 3.26)"
            echo "- ninja-build"
            echo "- pkg-config"
            exit 1
            ;;
    esac
}

# Install packages
install_packages

# Clone and bootstrap vcpkg
echo "Setting up vcpkg..."
if [ ! -d "vcpkg" ]; then
    git clone https://github.com/microsoft/vcpkg.git
fi

cd vcpkg
./bootstrap-vcpkg.sh
cd ..

# Install dependencies
echo "Installing vcpkg dependencies..."
./vcpkg/vcpkg install

echo "Setup complete!"
