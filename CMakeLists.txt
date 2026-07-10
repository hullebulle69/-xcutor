cmake_minimum_required(VERSION 3.20)
project(xcutor VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ── Luau – compiled as an OBJECT library ──────────────────────────────────────
#
# Linking Luau static libs into a SHARED target via the MSVC toolchain
# silently drops symbols (LNK2019) regardless of CMake generator.  Compiling
# Luau's source files directly as an OBJECT library and including the resulting
# .obj files in xcutor.dll's link command is the reliable alternative.
#
# Source lists are hard-coded from the current Luau tree so we don't silently
# miss files if file(GLOB) doesn't re-run after a Luau update.

set(LUAU_ROOT "${CMAKE_SOURCE_DIR}/extern/luau")

set(LUAU_COMMON_SRC
    ${LUAU_ROOT}/Common/src/BytecodeWire.cpp
    ${LUAU_ROOT}/Common/src/StringUtils.cpp
    ${LUAU_ROOT}/Common/src/TimeTrace.cpp
)

set(LUAU_AST_SRC
    ${LUAU_ROOT}/Ast/src/Allocator.cpp
    ${LUAU_ROOT}/Ast/src/Ast.cpp
    ${LUAU_ROOT}/Ast/src/Confusables.cpp
    ${LUAU_ROOT}/Ast/src/Cst.cpp
    ${LUAU_ROOT}/Ast/src/Lexer.cpp
    ${LUAU_ROOT}/Ast/src/Location.cpp
    ${LUAU_ROOT}/Ast/src/Parser.cpp
    ${LUAU_ROOT}/Ast/src/PrettyPrinter.cpp
)

set(LUAU_BYTECODE_SRC
    ${LUAU_ROOT}/Bytecode/src/BytecodeBuilder.cpp
    ${LUAU_ROOT}/Bytecode/src/BytecodeGraph.cpp
)

set(LUAU_COMPILER_SRC
    ${LUAU_ROOT}/Compiler/src/BuiltinFolding.cpp
    ${LUAU_ROOT}/Compiler/src/Builtins.cpp
    ${LUAU_ROOT}/Compiler/src/Compiler.cpp
    ${LUAU_ROOT}/Compiler/src/ConstantFolding.cpp
    ${LUAU_ROOT}/Compiler/src/CostModel.cpp
    ${LUAU_ROOT}/Compiler/src/TableShape.cpp
    ${LUAU_ROOT}/Compiler/src/Types.cpp
    ${LUAU_ROOT}/Compiler/src/ValueTracking.cpp
    ${LUAU_ROOT}/Compiler/src/lcode.cpp
)

set(LUAU_VM_SRC
    ${LUAU_ROOT}/VM/src/lapi.cpp
    ${LUAU_ROOT}/VM/src/laux.cpp
    ${LUAU_ROOT}/VM/src/lbaselib.cpp
    ${LUAU_ROOT}/VM/src/lbitlib.cpp
    ${LUAU_ROOT}/VM/src/lbuffer.cpp
    ${LUAU_ROOT}/VM/src/lbuflib.cpp
    ${LUAU_ROOT}/VM/src/lbuiltins.cpp
    ${LUAU_ROOT}/VM/src/lclass.cpp
    ${LUAU_ROOT}/VM/src/lclasslib.cpp
    ${LUAU_ROOT}/VM/src/lcorolib.cpp
    ${LUAU_ROOT}/VM/src/ldblib.cpp
    ${LUAU_ROOT}/VM/src/ldebug.cpp
    ${LUAU_ROOT}/VM/src/ldo.cpp
    ${LUAU_ROOT}/VM/src/lfunc.cpp
    ${LUAU_ROOT}/VM/src/lgc.cpp
    ${LUAU_ROOT}/VM/src/lgcdebug.cpp
    ${LUAU_ROOT}/VM/src/linit.cpp
    ${LUAU_ROOT}/VM/src/lintlib.cpp
    ${LUAU_ROOT}/VM/src/lmathlib.cpp
    ${LUAU_ROOT}/VM/src/lmem.cpp
    ${LUAU_ROOT}/VM/src/lnumprint.cpp
    ${LUAU_ROOT}/VM/src/lobject.cpp
    ${LUAU_ROOT}/VM/src/loslib.cpp
    ${LUAU_ROOT}/VM/src/lperf.cpp
    ${LUAU_ROOT}/VM/src/lstate.cpp
    ${LUAU_ROOT}/VM/src/lstring.cpp
    ${LUAU_ROOT}/VM/src/lstrlib.cpp
    ${LUAU_ROOT}/VM/src/ltable.cpp
    ${LUAU_ROOT}/VM/src/ltablib.cpp
    ${LUAU_ROOT}/VM/src/ltm.cpp
    ${LUAU_ROOT}/VM/src/ludata.cpp
    ${LUAU_ROOT}/VM/src/lutf8lib.cpp
    ${LUAU_ROOT}/VM/src/lveclib.cpp
    ${LUAU_ROOT}/VM/src/lvmexecute.cpp
    ${LUAU_ROOT}/VM/src/lvmload.cpp
    ${LUAU_ROOT}/VM/src/lvmutils.cpp
)

add_library(luau_embed OBJECT
    ${LUAU_COMMON_SRC}
    ${LUAU_AST_SRC}
    ${LUAU_BYTECODE_SRC}
    ${LUAU_COMPILER_SRC}
    ${LUAU_VM_SRC}
)

# Public: these include paths propagate to xcutor so our headers can include lua.h
target_include_directories(luau_embed PUBLIC
    ${LUAU_ROOT}/VM/include
    ${LUAU_ROOT}/Compiler/include
    ${LUAU_ROOT}/Ast/include
    ${LUAU_ROOT}/Bytecode/include
    ${LUAU_ROOT}/Common/include
)

# Private: internal headers needed to compile Luau source files
target_include_directories(luau_embed PRIVATE
    ${LUAU_ROOT}/VM/src
    ${LUAU_ROOT}/Compiler/src
    ${LUAU_ROOT}/Ast/src
    ${LUAU_ROOT}/Bytecode/src
    ${LUAU_ROOT}/Common/src
)

# Suppress all warnings from Luau sources – third-party code, not ours.
if(MSVC)
    target_compile_options(luau_embed PRIVATE /W0 /EHsc)
else()
    target_compile_options(luau_embed PRIVATE -w)
endif()

# ── DLL target ───────────────────────────────────────────────────────────────
add_library(xcutor SHARED
    src/dllmain.cpp
    src/lua_bridge.cpp
    src/game_state.cpp
)

target_include_directories(xcutor PRIVATE
    include
)

# luau_embed is an OBJECT library: CMake adds its .obj files directly to
# xcutor.dll's link command, bypassing static-lib linkage entirely.
target_link_libraries(xcutor PRIVATE luau_embed)

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
