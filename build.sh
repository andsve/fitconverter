#!/bin/bash

# Make script exit on first error
set -e

# Detect OS
OS="$(uname)"
echo "Building on: $OS"

CFLAGS="-Wall -Wextra -D_CRT_SECURE_NO_WARNINGS -DHAVE_STRCPY_S -DHAVE_FOPEN_S -DHAVE_FREAD_S -DHAVE_STRCAT_S -DHAVE_MEMCPY_S -DHAVE_STRNLEN_S -DHAVE_FTELLI64 -DHAVE_FSEEKI64 -DHAVE_FTELLO64 -DHAVE_FSEEKO64"
LDFLAGS="-mwindows -lcomdlg32 -municode"

# Add local TinyTIFF library
if [ -f "libTinyTIFF_Release.a" ]; then
    LDFLAGS="$LDFLAGS ./libTinyTIFF_Release.a"
else
    echo "Error: libTinyTIFF_Release.a not found"
    exit 1
fi
TARGET="fits_converter.exe"

# Set compiler and flags based on OS
if [[ "$OS" == "Darwin" ]]; then
    CC=x86_64-w64-mingw32-gcc
elif [[ "$OS" == "MINGW"* ]] || [[ "$OS" == "MSYS"* ]]; then
    CC=mingw32-gcc    
else
    echo "Unsupported operating system"
    exit 1
fi

# Clean any previous build
rm -f *.o $TARGET

# Build the program
echo "Building with $CC..."
$CC $CFLAGS -c main.c -o main.o
$CC main.o -o $TARGET $LDFLAGS

# Check if build succeeded
if [ -f "$TARGET" ]; then
    echo "Build successful! Created: $TARGET"
else
    echo "Build failed!"
    exit 1
fi 