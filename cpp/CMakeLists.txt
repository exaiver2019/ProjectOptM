# Project OptM - C++ build (Visual Studio 2022 or any CMake + MSVC/MinGW toolchain)
cmake_minimum_required(VERSION 3.20)
project(ProjectOptM LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IMGUI third_party/imgui)
add_executable(ProjectOptM WIN32
    src/main.cpp src/app.cpp src/system_info.cpp src/data.cpp src/json.cpp src/util.cpp
    src/processes.cpp src/tweaks.cpp src/optimizer.cpp src/checks.cpp src/frames.cpp src/updater.cpp src/tweakset.cpp src/detect.cpp
    ${IMGUI}/imgui.cpp ${IMGUI}/imgui_draw.cpp ${IMGUI}/imgui_tables.cpp ${IMGUI}/imgui_widgets.cpp
    ${IMGUI}/imgui_impl_win32.cpp ${IMGUI}/imgui_impl_dx11.cpp
    res/resource.rc)
target_include_directories(ProjectOptM PRIVATE src ${IMGUI})
target_compile_definitions(ProjectOptM PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
target_link_libraries(ProjectOptM PRIVATE d3d11 dxgi dwmapi shell32 advapi32 user32 comdlg32 powrprof winhttp bcrypt ole32 windowscodecs version)

if(MSVC)
    # single exe with no Visual C++ runtime to install; our manifest is in resource.rc
    set_property(TARGET ProjectOptM PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_options(ProjectOptM PRIVATE /MANIFEST:NO)
    target_compile_options(ProjectOptM PRIVATE /W4 /utf-8)
    target_compile_definitions(ProjectOptM PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_link_options(ProjectOptM PRIVATE -static -municode)
endif()
