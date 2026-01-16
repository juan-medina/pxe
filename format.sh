#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Juan Medina
# SPDX-License-Identifier: MIT
set -euo pipefail

if [ $# -ne 1 ]; then
    echo "Usage: $0 source-directory"
    exit 1
fi

SOURCE_DIR="$1"

# run clang-format in-place for all source files
find "$SOURCE_DIR" -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.c" \) -exec clang-format -i {} \;

echo "Formatted all source files in $SOURCE_DIR"
