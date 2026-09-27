#!/bin/sh
# Cross-compile from Linux/WSL with MinGW-w64.
set -e
cd "$(dirname "$0")"
mkdir -p build/obj
CXX=x86_64-w64-mingw32-g++
FLAGS="-std=c++17 -O2 -DUNICODE -D_UNICODE -DNOMINMAX -DWIN32_LEAN_AND_MEAN -D_WIN32_WINNT=0x0A00 -Isrc -Ithird_party/imgui"
for f in src/*.cpp third_party/imgui/imgui.cpp third_party/imgui/imgui_draw.cpp third_party/imgui/imgui_tables.cpp \
         third_party/imgui/imgui_widgets.cpp third_party/imgui/imgui_impl_win32.cpp third_party/imgui/imgui_impl_dx11.cpp; do
  $CXX $FLAGS -c "$f" -o "build/obj/$(basename "$f" .cpp).o"
done
(cd res && x86_64-w64-mingw32-windres --preprocessor=cpp resource.rc -O coff -o ../build/obj/resource.o)
$CXX -o build/ProjectOptM.exe build/obj/*.o -municode -mwindows -static -s \
     -ld3d11 -ldxgi -ldwmapi -lshell32 -ladvapi32 -luser32 -lgdi32 -limm32 -ld3dcompiler \
     -lcomdlg32 -lpowrprof -lwinhttp -lbcrypt -lole32 -lwindowscodecs -lversion
echo "built build/ProjectOptM.exe"
