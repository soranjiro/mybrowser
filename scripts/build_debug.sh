#!/bin/bash
# Debug build script for MyBrowser

echo "Building MyBrowser in Debug mode..."

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    mkdir build
fi

cd build

# Configure with Debug mode
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Build the project
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

if [ $? -eq 0 ]; then
    echo "✅ Debug build completed successfully!"
    echo "📍 Executable: ./build/MyBrowser"
    echo "🏠 Homepage: debug_test.html (for testing)"
    echo "📊 Logging: Extensive debug logging enabled"
else
    echo "❌ Build failed!"
    exit 1
fi
