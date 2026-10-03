cmake_minimum_required(VERSION 3.21)

# LastSeed.dll - the SKSE plugin behind Last Seed 2026's HUD bars and SKSE Menu Framework page.
project(LastSeed VERSION 1.0.0 LANGUAGES CXX)

find_package(CommonLibSSE CONFIG REQUIRED)

add_commonlibsse_plugin(${PROJECT_NAME} NAME "LastSeed" AUTHOR "CageTV" SOURCES
    src/main.cpp
    src/Settings.cpp
    src/Game.cpp
    src/Hud.cpp
    src/Menu.cpp
    src/AutoStart.cpp
    src/Spoilage.cpp
    src/Freshness.cpp
    src/GridInv.cpp
    src/NativeMcm.cpp
    src/Hotkeys.cpp
    src/NativeFoodLists.cpp
    src/Compat.cpp
    src/Ids.cpp
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_include_directories(${PROJECT_NAME} PRIVATE src include)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)
target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:/Z7>)
target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:/DEBUG /OPT:REF /OPT:ICF>)
