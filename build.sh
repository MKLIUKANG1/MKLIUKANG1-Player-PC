#!/bin/bash
set -e
cd "$(dirname "$0")"

BUILD=build
mkdir -p "$BUILD"

echo "[1/5] Packer -> resources.pak"
g++ -std=c++17 -O2 packer.cpp -o packer.exe -lz
./packer.exe

echo "[2/5] windres -> resources.o"
windres resources.rc -O coff -o resources.o

echo "[3/5] Compile player"
g++ -std=c++17 -O2 -mwindows main.cpp vgm_export.cpp resources.o \
    -o "$BUILD/MKLK1Player.exe" \
    $(pkg-config --cflags --libs sdl2 fluidsynth) -lz

echo "[4/5] Copy runtime files to $BUILD"
cp -f icon.png         "$BUILD/" 2>/dev/null || true
cp -f xg.wopn          "$BUILD/" 2>/dev/null || true
cp -f midi2vgm_opn2.exe "$BUILD/" 2>/dev/null || true
cp -f mid2vgm_adl.exe   "$BUILD/" 2>/dev/null || true
[ -d isc ] && cp -rf isc "$BUILD/" || true

echo "[5/5] Copy DLL dependencies"
cd "$BUILD"
MINGW=/mingw64/bin
for pass in 1 2 3 4; do
    MISSING=$(ntldd -R MKLK1Player.exe 2>/dev/null \
        | grep "not found" \
        | grep -v "ext-ms-\|api-ms-\|PdmUtilities\|HvsiFileTrust" \
        | awk '{print $1}' | sort -u)
    [ -z "$MISSING" ] && break
    for dll in $MISSING; do
        [ -f "$MINGW/$dll" ] && cp -n "$MINGW/$dll" .
    done
done

echo "Done: $(pwd)/MKLK1Player.exe"
echo "DLLs: $(ls -1 *.dll 2>/dev/null | wc -l)"
