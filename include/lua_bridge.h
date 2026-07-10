#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>

// Luau C++ API (Luau is a C++ library — no extern "C" wrapper)
#include "lua.h"
#include "lualib.h"
#include "luacode.h"  // luau_compile / luau_load

#ifdef XCUTOR_EXPORTS
#  define XCUTOR_API __declspec(dllexport)
#else
#  define XCUTOR_API __declspec(dllimport)
#endif

// ---------------------------------------------------------------------------
// LuaBridge
//
// Owns one Luau VM state. All public methods are safe to call from any thread
// (they acquire vm_mutex).  The VM itself is single-threaded; calls serialise
// at the mutex boundary.
// ---------------------------------------------------------------------------
class XCUTOR_API LuaBridge {
public:
    LuaBridge()  = default;
    ~LuaBridge() { shutdown(); }

    LuaBridge(const LuaBridge&)            = delete;
    LuaBridge& operator=(const LuaBridge&) = delete;

    // ── Lifecycle ─────────────────────────────────────────────────────────
    bool init();
    void shutdown();
    bool is_alive() const { return L != nullptr; }

    // ── Script loading ─────────────────────────────────────────────────────
    // Compile Lua source to Luau bytecode and push it as a chunk function.
    // The chunk sits on the Lua stack ready for execute().
    bool load(std::string_view source, std::string_view chunk_name = "chunk");

    // Call the chunk that load() pushed. Pops it regardless of outcome.
    // Returns false and stores an error message in last_error on failure.
    bool execute();

    // Convenience: load + execute in one call.
    bool run(std::string_view source, std::string_view chunk_name = "chunk");

    // ── Global state manipulation ──────────────────────────────────────────
    // Write primitive values into the VM's global namespace.
    void set_global_int   (const char* name, lua_Integer value);
    void set_global_number(const char* name, lua_Number  value);
    void set_global_string(const char* name, const char* value);
    void set_global_bool  (const char* name, bool        value);
    void set_global_nil   (const char* name);

    // Read a global back out.  Returns false if the key is absent or
    // has the wrong type; *out is untouched on failure.
    bool get_global_int   (const char* name, lua_Integer& out);
    bool get_global_number(const char* name, lua_Number&  out);
    bool get_global_string(const char* name, std::string& out);

    // ── Function binding ───────────────────────────────────────────────────
    // Register a C function as a Lua global (e.g. "GetHealth").
    void register_fn(const char* name, lua_CFunction fn);

    // Register an entire table of functions at once.
    struct FnReg { const char* name; lua_CFunction fn; };
    void register_lib(const char* table_name, const FnReg* regs, std::size_t count);

    // ── Diagnostics ───────────────────────────────────────────────────────
    const std::string& last_error() const { return error_msg; }

    // Attach a sink that receives output from the Lua print() function.
    using PrintSink = std::function<void(std::string_view)>;
    void set_print_sink(PrintSink sink);

    // Raw access – use with caution; caller must hold the lock.
    lua_State* raw_state() { return L; }
    std::mutex& lock()     { return vm_mutex; }

private:
    lua_State* L = nullptr;
    mutable std::mutex vm_mutex;
    std::string error_msg;
    PrintSink   print_sink;

    // Redirect Lua's print() to our sink.
    void install_print_override();

    // Pull the top-of-stack error string into error_msg and pop it.
    void capture_error();
};
