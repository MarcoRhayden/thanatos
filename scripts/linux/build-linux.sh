#!/bin/bash

# Default values
BUILD_TYPE="Release"
BUILD_DIR="build"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [--debug] [--build-dir DIR]"
            echo "  --debug      Build in Debug mode (default: Release)"
            echo "  --build-dir  Specify build directory (default: build)"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

echo "Building Thanatos for Linux..."
echo "Build type: $BUILD_TYPE"
echo "Build directory: $BUILD_DIR"

# Configure with CMake
echo "Configuring with CMake..."
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake \
    -G Ninja

# Build
echo "Building..."
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE"

echo "Build complete! Executable location: $BUILD_DIR/Thanatos"
