#pragma once

#include <cstdint>
#include <string_view>

#ifdef XCUTOR_EXPORTS
#  define XCUTOR_API __declspec(dllexport)
#else
#  define XCUTOR_API __declspec(dllimport)
#endif

// Forward declaration – avoid pulling Lua headers into every translation unit.
struct lua_State;

// ---------------------------------------------------------------------------
// Memory primitives
//
// When the DLL is injected into a target process, these helpers read and write
// its virtual address space directly.  When running out-of-process (external
// cheat / tool), swap the bodies for ReadProcessMemory / WriteProcessMemory
// calls against a stored HANDLE.
// ---------------------------------------------------------------------------
namespace mem {

template<typename T>
inline T read(uintptr_t addr) noexcept {
    return *reinterpret_cast<const T*>(addr);
}

template<typename T>
inline void write(uintptr_t addr, T value) noexcept {
    *reinterpret_cast<T*>(addr) = value;
}

// Walk a multi-level pointer chain: base → [off0] → [off1] → … → final addr.
inline uintptr_t resolve_ptr_chain(uintptr_t base,
                                   const uintptr_t* offsets,
                                   std::size_t      depth) noexcept {
    uintptr_t addr = base;
    for (std::size_t i = 0; i < depth; ++i) {
        addr = mem::read<uintptr_t>(addr);
        if (!addr) return 0;
        addr += offsets[i];
    }
    return addr;
}

} // namespace mem

// ---------------------------------------------------------------------------
// GameState
//
// Centralises all knowledge about the target application's global state.
// Fill in the offsets for your specific target.  The Lua bindings below
// expose a clean "game" table to scripts.
// ---------------------------------------------------------------------------
class XCUTOR_API GameState {
public:
    // ── Singleton ────────────────────────────────────────────────────────
    static GameState& get() {
        static GameState instance;
        return instance;
    }

    // ── Initialisation ───────────────────────────────────────────────────
    // Call once after the DLL attaches and the target module has loaded.
    bool init(uintptr_t module_base);
    void shutdown();

    // ── Player entity reads ───────────────────────────────────────────────
    float        player_health()   const noexcept;
    float        player_max_health() const noexcept;
    float        player_stamina()  const noexcept;
    bool         player_is_alive() const noexcept;
    uintptr_t    player_ptr()      const noexcept { return cached_player_ptr; }

    // ── Player entity writes ──────────────────────────────────────────────
    void set_player_health(float v)   noexcept;
    void set_player_stamina(float v)  noexcept;

    // ── World state reads ─────────────────────────────────────────────────
    int    current_level()  const noexcept;
    double game_time()      const noexcept;
    bool   is_paused()      const noexcept;

    // ── Lua binding ───────────────────────────────────────────────────────
    // Register a "game" global table in L that exposes the getters/setters.
    void bind_to_lua(lua_State* L);

    // ── Offset table (edit for your target) ──────────────────────────────
    struct Offsets {
        // Module-relative static pointer to the local player object.
        uintptr_t static_player_ptr = 0x00000000;

        // Fields inside the player object.
        uintptr_t health      = 0x100;
        uintptr_t max_health  = 0x104;
        uintptr_t stamina     = 0x108;

        // Module-relative statics.
        uintptr_t level       = 0x00000000;
        uintptr_t game_time   = 0x00000000;
        uintptr_t is_paused   = 0x00000000;
    } offsets;

private:
    GameState() = default;

    uintptr_t module_base        = 0;
    uintptr_t cached_player_ptr  = 0;
    bool      initialised        = false;

    // Refresh the cached dynamic player pointer (call each frame if needed).
    void refresh_player_ptr() noexcept;

    // ── Lua C-function implementations ───────────────────────────────────
    static int lua_get_health    (lua_State* L);
    static int lua_set_health    (lua_State* L);
    static int lua_get_stamina   (lua_State* L);
    static int lua_set_stamina   (lua_State* L);
    static int lua_is_alive      (lua_State* L);
    static int lua_get_level     (lua_State* L);
    static int lua_get_game_time (lua_State* L);
    static int lua_is_paused     (lua_State* L);
};
