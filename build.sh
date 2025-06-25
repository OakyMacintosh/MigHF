#!/bin/bash


# check if ./build exists
if [ ! -d "./build" ]; then
    echo "Creating build directory..."
    mkdir -p "./build"
else
    echo "Build directory already exists."
fi

# check if ./build/win-msys2 exists
if [ ! -d "./build/win-msys2" ]; then
    echo "Creating build/win-msys2 directory..."
    mkdir -p "./build/win-msys2/dll"
    mkdir -p "./build/bin"
    bash msys2.sh
    echo "Build/win-msys2 directory created and DLLs copied."
else
    echo "Build/win-msys2 directory already exists."
fi

# check if ./build/win-msys2/dll exists
if [ ! -d "./build/win-msys2/dll" ]; then
    echo "Creating build/win-msys2/dll directory..."
    mkdir -p "./build/win-msys2/dll"
else
    echo "Build/win-msys2/dll directory already exists."
fi

# check if gcc is installed
if ! command -v gcc &> /dev/null; then
    echo "GCC is not installed. Please install GCC to continue."
    exit 1
else
    gcc wozclang.c -o ./build/bin/wozclang
    if [ $? -ne 0 ]; then
        echo "GCC compilation failed."
        exit 1
    else
        echo "GCC compilation succeeded."
        continue-1() {
            gcc source/mighf.c -o ./build/bin/mighf-$PLATFORM
        }
        continue-1
        if [ $? -ne 0 ]; then
            echo "GCC compilation for mighf failed."
            exit 1
        else
            echo "GCC compilation for mighf succeeded."
        fi
    fi
fi

# check if nasm is installed
if ! command -v nasm &> /dev/null; then
    echo "NASM is not installed. Please install NASM to continue."
    exit 1
else
    nasm woz.asm86 -f $PLATFORMNASM -o ./build/bin/woz.$PLATFORMNASM
    if [ $? -ne 0 ]; then
        echo "NASM compilation failed."
        exit 1
    else
        echo "NASM compilation succeeded."
    fi
fi