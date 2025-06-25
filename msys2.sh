#!/bin/bash
set -e

# Directory to copy DLLs to
DEST_DIR="./build/win-msys2/dll"
mkdir -p "$DEST_DIR"

# List of required DLLs for MSYS2-compiled programs
REQUIRED_DLLS=(
    msys-2.0.dll
    msys-gcc_s-seh-1.dll
    msys-stdc++-6.dll
    msys-intl-8.dll
    msys-iconv-2.dll
    msys-z.dll
    msys-bz2-1.dll
    msys-winpthread-1.dll
)

# Find MSYS2 DLL directory
MSYS2_DLL_DIR="/usr/bin"

# Add NASM DLL dependencies if nasm is available
if command -v nasm >/dev/null 2>&1; then
    NASM_BIN=$(command -v nasm)
    NASM_DLLS=$(ldd "$NASM_BIN" | awk '/msys-.*\.dll/ {print $1}' | sort -u)
    for dll in $NASM_DLLS; do
        # Only add if not already in the list
        if [[ ! " ${REQUIRED_DLLS[@]} " =~ " $dll " ]]; then
            REQUIRED_DLLS+=("$dll")
        fi
    done
fi

for dll in "${REQUIRED_DLLS[@]}"; do
    src="$MSYS2_DLL_DIR/$dll"
    if [[ -f "$src" ]]; then
        cp -u "$src" "$DEST_DIR/"
        echo "Copied $dll"
    else
        echo "Warning: $dll not found in $MSYS2_DLL_DIR"
    fi
done