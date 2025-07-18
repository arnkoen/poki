#!/bin/bash 2>nul || goto :windows

# -------- BASH (Linux) --------
BUILD_TYPE="Debug"
CFLAGS="-g -O0 -Wall -Wextra"
LIBS="-lX11 -lXi -lXcursor -lGL -ldl -lpthread -lm -lasound"

if [ "$1" == "--release" ]; then
    BUILD_TYPE="Release"
    CFLAGS="-O2 -DNDEBUG"
fi

CFLAGS="$CFLAGS -DSOKOL_GLCORE -I."

mkdir -p out

echo "[BASH] Build type: $BUILD_TYPE"
echo Compiling poki...
gcc -c poki.c -o out/poki.o $CFLAGS
gcc -c deps.c -o out/deps.o $CFLAGS
ar rcs out/libpoki.a out/poki.o out/deps.o

echo Building viewgltf...
gcc viewgltf.c -o out/viewgltf out/libpoki.a $CFLAGS $LIBS

echo Building viewm3d...
gcc viewm3d.c -o out/viewm3d out/libpoki.a $CFLAGS $LIBS

echo Building playsound...
gcc poki.c deps.c playsounds.c -o out/playsound $CFLAGS $LIBS

echo Copying assets...
cp -r assets out/

exit 0

:windows
@echo off
setlocal

:: -------- WINDOWS (MSVC) --------
set CFLAGS=/DSOKOL_GLCORE /I. /Foout\ /Fdout\poki.pdb
set LINKLIBS=opengl32.lib user32.lib gdi32.lib shell32.lib

set BUILD_TYPE=Debug
set CFLAGS=%CFLAGS% /Zi /Od

if "%1"=="--release" (
    set BUILD_TYPE=Release
    set CFLAGS=/DSOKOL_D3D11 /I. /O2 /DNDEBUG /Foout\ /Fdout\poki.pdb
)

echo [WINDOWS] Build type: %BUILD_TYPE%

if not exist out mkdir out

echo Compiling poki...
cl /nologo /c poki.c %CFLAGS%
cl /nologo /c deps.c %CFLAGS%
lib /nologo /OUT:out\poki.lib out\poki.obj out\deps.obj

echo Building viewgltf...
cl /nologo /c viewgltf.c %CFLAGS%
cl /nologo out\viewgltf.obj out\poki.lib %CFLAGS% %LINKLIBS% /Feout\viewgltf.exe /DPK_NO_AUDIO

echo Building viewm3d...
cl /nologo /c viewm3d.c %CFLAGS%
cl /nologo out\viewm3d.obj out\poki.lib %CFLAGS% %LINKLIBS% /Feout\viewm3d.exe /DPK_NO_AUDIO

echo Building playsounds...
cl /nologo poki.c deps.c playsounds.c %CFLAGS% %LINKLIBS% /Feout\playsound.exe /DPK_NO_SAPP

echo Copying assets...
xcopy /E /I /Y assets out\assets >nul

endlocal
exit /b
