# Build Scripts

This directory contains build scripts for MyBrowser.

## Scripts

### `build_debug.sh`

Builds MyBrowser in debug mode with debugging symbols and verbose output.

### `build_release.sh`

Builds MyBrowser in release mode with optimizations.

## Usage

From the project root directory:

```bash
# Debug build
./scripts/build_debug.sh

# Release build
./scripts/build_release.sh
```

## Requirements

- Qt 5.15 or later
- CMake 3.16 or later
- C++17 compatible compiler
- Qt WebEngine development packages

## Build Output

Built executables will be placed in the `build/` directory in the project root.
