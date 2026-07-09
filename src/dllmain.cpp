#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "lua_bridge.h"
#include "game_state.h"

// ---------------------------------------------------------------------------
// Module-level bridge instance
//
// Keeping a single LuaBridge per DLL load is the typical pattern.  If you
// need scripted concurrency, create one lua_State per thread and synchronise
// at the GameState layer instead.
// ---------------------------------------------------------------------------
static LuaBridge g_bridge;

// ---------------------------------------------------------------------------
// DLL attach/detach
// ---------------------------------------------------------------------------
static void on_attach(HMODULE module) {
    // Optional: open a console for debug output.
#ifdef _DEBUG
    AllocConsole();
    FILE* dummy;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);
#endif

    // Route Lua print() to the Win32 debug output; swap for your own sink.
    g_bridge.set_print_sink([](std::string_view msg) {
        OutputDebugStringA(msg.data());
        OutputDebugStringA("\n");
    });

    if (!g_bridge.init()) {
        OutputDebugStringA("[xcutor] LuaBridge::init() failed\n");
        return;
    }

    // ── Bind the target application's state ─────────────────────────────
    // Replace 0 with GetModuleHandle("game.exe") or the actual base address
    // obtained from a pattern scan.
    const uintptr_t game_base = reinterpret_cast<uintptr_t>(
        GetModuleHandleA(nullptr)  // main module – update for the real target
    );

    GameState::get().init(game_base);
    GameState::get().bind_to_lua(g_bridge.raw_state());

    // ── Run an initial bootstrap script ──────────────────────────────────
    // In production this might load from a file or a remote URL.
    constexpr std::string_view bootstrap = R"lua(
        print("xcutor Lua environment ready")
        print("Player alive:", game.isAlive())
        print("Level:", game.getLevel())
    )lua";

    if (!g_bridge.run(bootstrap, "bootstrap")) {
        OutputDebugStringA("[xcutor] bootstrap script failed: ");
        OutputDebugStringA(g_bridge.last_error().c_str());
        OutputDebugStringA("\n");
    }
}

static void on_detach() {
    GameState::get().shutdown();
    g_bridge.shutdown();
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD   ul_reason_for_call,
                      LPVOID  /*lpReserved*/)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        // DllMain is called with the loader lock held – heavy work must be
        // deferred to a new thread to avoid deadlocks.
        CreateThread(nullptr, 0,
            [](LPVOID param) -> DWORD {
                on_attach(static_cast<HMODULE>(param));
                return 0;
            },
            hModule, 0, nullptr);
        break;

    case DLL_PROCESS_DETACH:
        on_detach();
        break;

    default:
        break;
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
// Exported C API
//
// Lets an external loader call into the bridge without C++ name mangling.
// ---------------------------------------------------------------------------
extern "C" {

XCUTOR_API bool xcutor_run_script(const char* source, const char* chunk_name) {
    if (!source) return false;
    return g_bridge.run(source, chunk_name ? chunk_name : "remote");
}

XCUTOR_API const char* xcutor_last_error() {
    return g_bridge.last_error().c_str();
}

XCUTOR_API void xcutor_set_global_number(const char* name, double value) {
    g_bridge.set_global_number(name, value);
}

XCUTOR_API void xcutor_set_global_string(const char* name, const char* value) {
    g_bridge.set_global_string(name, value);
}

} // extern "C"
