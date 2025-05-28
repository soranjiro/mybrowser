#!/bin/bash
# Release build script for MyBrowser

echo "Building MyBrowser in Release mode..."

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    mkdir build
fi

cd build

# Configure with Release mode
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build the project
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

if [ $? -eq 0 ]; then
    echo "✅ Release build completed successfully!"
    echo "📍 Executable: ./build/MyBrowser"
    echo "🏠 Homepage: https://www.google.com"
    echo "📊 Logging: Minimal logging (errors only)"
else
    echo "❌ Build failed!"
    exit 1
fi
