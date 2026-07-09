#include "lua_bridge.h"

#include <cassert>
#include <cstdlib>  // std::free

// ---------------------------------------------------------------------------
// Luau allocator
//
// Using a custom allocator lets us track live VM memory.  The default Luau
// allocator calls realloc(); mirror that exactly so behaviour is identical,
// but the hook is in place for future instrumentation.
// ---------------------------------------------------------------------------
static void* luau_alloc(void* /*ud*/, void* ptr,
                        std::size_t /*old_size*/, std::size_t new_size) noexcept
{
    if (new_size == 0) {
        std::free(ptr);
        return nullptr;
    }
    return std::realloc(ptr, new_size);
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------
bool LuaBridge::init() {
    std::unique_lock lock(vm_mutex);

    if (L) return true; // already alive

    L = lua_newstate(luau_alloc, nullptr);
    if (!L) return false;

    // Open safe standard libraries.  Omit io/os/ffi for sandboxed scripts.
    luaL_openlibs(L);

    install_print_override();
    return true;
}

void LuaBridge::shutdown() {
    std::unique_lock lock(vm_mutex);
    if (L) {
        lua_close(L);
        L = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Script loading
// ---------------------------------------------------------------------------
bool LuaBridge::load(std::string_view source, std::string_view chunk_name) {
    assert(L && "LuaBridge::init() not called");
    std::unique_lock lock(vm_mutex);

    // ── Step 1: compile source → Luau bytecode ───────────────────────────
    // luau_compile() is Luau-specific.  It returns a heap buffer that must be
    // freed with std::free().  Standard Lua would use luaL_loadbuffer here.
    std::size_t bytecode_size = 0;
    char* bytecode = luau_compile(source.data(), source.size(),
                                  nullptr,        // default compile options
                                  &bytecode_size);
    if (!bytecode) {
        error_msg = "luau_compile returned null";
        return false;
    }

    // ── Step 2: load bytecode into the VM ────────────────────────────────
    // On success luau_load pushes a function (the chunk) onto the stack.
    // On failure it pushes an error string instead.
    const int rc = luau_load(L,
                             chunk_name.data(),
                             bytecode,
                             bytecode_size,
                             /*env=*/0);   // 0 = use the global environment
    std::free(bytecode);

    if (rc != LUA_OK) {
        capture_error();
        return false;
    }
    return true; // chunk function is now on top of the stack
}

bool LuaBridge::execute() {
    assert(L && "LuaBridge::init() not called");
    std::unique_lock lock(vm_mutex);

    // The chunk pushed by load() is at the top of the stack.
    // lua_pcall(L, nargs=0, nresults=LUA_MULTRET, errhandler=0)
    const int rc = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (rc != LUA_OK) {
        capture_error();
        return false;
    }
    // Discard any return values left on the stack.
    lua_settop(L, 0);
    return true;
}

bool LuaBridge::run(std::string_view source, std::string_view chunk_name) {
    // load() acquires vm_mutex internally; execute() does too.
    // We deliberately keep them separate so callers can load once, then
    // execute() multiple times against a re-set global state.
    return load(source, chunk_name) && execute();
}

// ---------------------------------------------------------------------------
// Global state – writes
// ---------------------------------------------------------------------------
void LuaBridge::set_global_int(const char* name, lua_Integer value) {
    std::unique_lock lock(vm_mutex);
    lua_pushinteger(L, value);
    lua_setglobal(L, name);
}

void LuaBridge::set_global_number(const char* name, lua_Number value) {
    std::unique_lock lock(vm_mutex);
    lua_pushnumber(L, value);
    lua_setglobal(L, name);
}

void LuaBridge::set_global_string(const char* name, const char* value) {
    std::unique_lock lock(vm_mutex);
    lua_pushstring(L, value);
    lua_setglobal(L, name);
}

void LuaBridge::set_global_bool(const char* name, bool value) {
    std::unique_lock lock(vm_mutex);
    lua_pushboolean(L, value ? 1 : 0);
    lua_setglobal(L, name);
}

void LuaBridge::set_global_nil(const char* name) {
    std::unique_lock lock(vm_mutex);
    lua_pushnil(L);
    lua_setglobal(L, name);
}

// ---------------------------------------------------------------------------
// Global state – reads
// ---------------------------------------------------------------------------
bool LuaBridge::get_global_int(const char* name, lua_Integer& out) {
    std::unique_lock lock(vm_mutex);
    lua_getglobal(L, name);
    if (!lua_isinteger(L, -1)) { lua_pop(L, 1); return false; }
    out = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return true;
}

bool LuaBridge::get_global_number(const char* name, lua_Number& out) {
    std::unique_lock lock(vm_mutex);
    lua_getglobal(L, name);
    if (!lua_isnumber(L, -1)) { lua_pop(L, 1); return false; }
    out = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return true;
}

bool LuaBridge::get_global_string(const char* name, std::string& out) {
    std::unique_lock lock(vm_mutex);
    lua_getglobal(L, name);
    if (!lua_isstring(L, -1)) { lua_pop(L, 1); return false; }
    out = lua_tostring(L, -1);
    lua_pop(L, 1);
    return true;
}

// ---------------------------------------------------------------------------
// Function binding
// ---------------------------------------------------------------------------
void LuaBridge::register_fn(const char* name, lua_CFunction fn) {
    std::unique_lock lock(vm_mutex);
    lua_pushcfunction(L, fn, name);
    lua_setglobal(L, name);
}

void LuaBridge::register_lib(const char* table_name,
                              const FnReg* regs,
                              std::size_t  count) {
    std::unique_lock lock(vm_mutex);
    lua_newtable(L);
    for (std::size_t i = 0; i < count; ++i) {
        lua_pushcfunction(L, regs[i].fn, regs[i].name);
        lua_setfield(L, -2, regs[i].name);
    }
    lua_setglobal(L, table_name);
}

// ---------------------------------------------------------------------------
// PrintSink
// ---------------------------------------------------------------------------
void LuaBridge::set_print_sink(PrintSink sink) {
    std::unique_lock lock(vm_mutex);
    print_sink = std::move(sink);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

// The Lua print() override lives in a free function so it can be a plain
// lua_CFunction.  It captures 'this' via the upvalue mechanism.
static int lua_print_override(lua_State* L) {
    // Retrieve the LuaBridge pointer from upvalue 1.
    auto* bridge = static_cast<LuaBridge*>(lua_touserdata(L, lua_upvalueindex(1)));

    const int n = lua_gettop(L);
    std::string line;
    line.reserve(128);

    for (int i = 1; i <= n; ++i) {
        if (i > 1) line += '\t';
        std::size_t len = 0;
        const char* s = luaL_tolstring(L, i, &len); // converts to string
        line.append(s, len);
        lua_pop(L, 1); // pop the string luaL_tolstring pushed
    }

    if (bridge && bridge->set_print_sink, true) {
        // The sink is private; call via the public accessor pattern below.
        // We stash the sink pointer itself as a second upvalue instead.
    }

    // Upvalue 2 is a lightuserdata pointing to the PrintSink object.
    auto* sink_ptr = static_cast<LuaBridge::PrintSink*>(
        lua_touserdata(L, lua_upvalueindex(2)));
    if (sink_ptr && *sink_ptr) {
        (*sink_ptr)(line);
    }
    return 0;
}

void LuaBridge::install_print_override() {
    // Push upvalue 1: 'this' (the LuaBridge).
    lua_pushlightuserdata(L, this);
    // Push upvalue 2: pointer to our PrintSink member.
    lua_pushlightuserdata(L, &print_sink);
    // Create the closure with 2 upvalues.
    lua_pushcclosure(L, lua_print_override, "print", 2);
    lua_setglobal(L, "print");
}

void LuaBridge::capture_error() {
    if (lua_gettop(L) > 0 && lua_isstring(L, -1)) {
        error_msg = lua_tostring(L, -1);
    } else {
        error_msg = "(unknown Lua error)";
    }
    lua_pop(L, 1);
}
