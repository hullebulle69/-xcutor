cmake_minimum_required(VERSION 3.20)
project(xcutor VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ── Luau ─────────────────────────────────────────────────────────────────────
# Clone with: git submodule add https://github.com/luau-lang/luau.git extern/luau
set(LUAU_BUILD_CLI  OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_WEB  OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/luau)

# ── DLL target ───────────────────────────────────────────────────────────────
add_library(xcutor SHARED
    src/dllmain.cpp
    src/lua_bridge.cpp
    src/game_state.cpp
)

target_include_directories(xcutor PRIVATE
    include
    extern/luau/VM/include
    extern/luau/Compiler/include
    extern/luau/Common/include
)

# Use file-path generator expressions instead of target names to work around
# the MSVC VS-generator's project-reference mechanism not passing static libs
# into the SHARED target's link command.  We also enumerate the full transitive
# closure manually because file-path entries don't propagate PUBLIC deps:
#   Luau.VM        → Luau.Common
#   Luau.Compiler  → Luau.Ast → Luau.Common
#                  → Luau.Bytecode → Luau.Common
add_dependencies(xcutor Luau.VM Luau.Compiler Luau.Ast Luau.Bytecode Luau.Common)
target_link_libraries(xcutor PRIVATE
    "$<TARGET_LINKER_FILE:Luau.VM>"
    "$<TARGET_LINKER_FILE:Luau.Compiler>"
    "$<TARGET_LINKER_FILE:Luau.Ast>"
    "$<TARGET_LINKER_FILE:Luau.Bytecode>"
    "$<TARGET_LINKER_FILE:Luau.Common>"
)

target_compile_definitions(xcutor PRIVATE
    XCUTOR_EXPORTS
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if(MSVC)
    target_compile_options(xcutor PRIVATE /W4 /WX /EHsc /wd4251 /wd4100)
else()
    target_compile_options(xcutor PRIVATE -Wall -Wextra -Werror)
endif()

# ── Injector executable ───────────────────────────────────────────────────────
add_subdirectory(injector)
