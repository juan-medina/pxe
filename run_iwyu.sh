#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Juan Medina
# SPDX-License-Identifier: MIT

set -e

# --- 1. Validate arguments ---
if [ $# -ne 1 ]; then
    echo "Usage: $0 <build-directory>"
    exit 1
fi

BUILD_DIR="$1"

# --- 2. Check compile_commands.json exists ---
if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
    echo "Error: $BUILD_DIR/compile_commands.json not found."
    exit 1
fi

# --- 3. Ensure we are run from project root ---
if [ ! -d "src" ]; then
    echo "Error: run this script from the project root (where src/ exists)."
    exit 1
fi

# --- 4. Collect all project source files (relative paths) ---
SRC_FILES=$(find src -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \))

# --- 5. Run IWYU safely ---
iwyu_tool.py -p "$BUILD_DIR" $SRC_FILES | python3 /usr/local/bin/fix_includes.py

# --- 6. Reformat all files to restore include order ---
for f in $SRC_FILES; do
    clang-format -i "$f"
done
