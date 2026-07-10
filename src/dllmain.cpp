#include <windows.h>

#include <cstdint>
#include <string>
#include <string_view>

#include "lua_bridge.h"
#include "game_state.h"

// ---------------------------------------------------------------------------
// XOR cipher — rolling 8-byte key applied to raw source bytes
// Must match XorKey in MainWindow.xaml.cs
// ---------------------------------------------------------------------------
static constexpr uint8_t kXorKey[] = { 0x5A, 0x9C, 0x3F, 0xA1, 0x77, 0xE4, 0x2D, 0xB8 };

static void xor_apply(std::string& data) {
    for (std::size_t i = 0; i < data.size(); ++i)
        data[i] = static_cast<char>(static_cast<uint8_t>(data[i]) ^ kXorKey[i % sizeof(kXorKey)]);
}

// ---------------------------------------------------------------------------
// Base64 decoder
// ---------------------------------------------------------------------------
static std::string base64_decode(std::string_view input) {
    // Maps ASCII byte → 6-bit base64 value; 255 = not a base64 character.
    static constexpr uint8_t kDec[128] = {
        255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, // 0-15
        255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, // 16-31
        255,255,255,255,255,255,255,255,255,255,255, 62,255,255,255, 63, // 32-47  (+, /)
         52, 53, 54, 55, 56, 57, 58, 59, 60, 61,255,255,255,255,255,255, // 48-63  (0-9)
        255,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, // 64-79  (A-O)
         15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,255,255,255,255,255, // 80-95  (P-Z)
        255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, // 96-111 (a-o)
         41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,255,255,255,255,255, // 112-127 (p-z)
    };

    std::string out;
    out.reserve(input.size() * 3 / 4 + 2);

    uint32_t acc  = 0;
    int      bits = 0;

    for (unsigned char c : input) {
        if (c == '=') break;
        if (c >= 128) continue;
        const uint8_t v = kDec[c];
        if (v == 255) continue;
        acc   = (acc << 6) | v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out += static_cast<char>((acc >> bits) & 0xFF);
        }
    }
    return out;
}

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
// RemoteScriptArgs
//
// Argument block written into the target process by the WPF injector.
// xcutor_execute_remote receives a pointer to this struct as its thread param.
// ---------------------------------------------------------------------------
struct RemoteScriptArgs {
    char source[0x8000];    // up to 32 KB — accommodates double-base64 overhead
    char chunk_name[64];    // name shown in error messages
};

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

// Called via CreateRemoteThread from the WPF UI.  The parameter is a pointer
// to a RemoteScriptArgs struct allocated (and later freed) by the caller.
// The source field carries the Lua script as base64-encoded UTF-8.
XCUTOR_API DWORD WINAPI xcutor_execute_remote(LPVOID param) {
    if (!param) return 1;
    const auto* args = static_cast<const RemoteScriptArgs*>(param);

    // Decode pipeline (reverse of UI encoding): base64 → base64 → XOR
    const std::string_view outer(args->source,
                                 strnlen(args->source, sizeof(args->source)));
    const std::string inner = base64_decode(outer);
    std::string       source = base64_decode(std::string_view(inner));
    xor_apply(source);

    const bool ok = g_bridge.run(
        std::string_view(source.data(), source.size()),
        std::string_view(args->chunk_name,
                         strnlen(args->chunk_name, sizeof(args->chunk_name)))
    );
    return ok ? 0 : 1;
}

} // extern "C"
