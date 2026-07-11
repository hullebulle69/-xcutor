#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "offsets.h"

#ifdef XCUTOR_EXPORTS
#  define XCUTOR_API __declspec(dllexport)
#else
#  define XCUTOR_API __declspec(dllimport)
#endif

struct lua_State;

// ---------------------------------------------------------------------------
// Memory primitives
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

} // namespace mem

// ---------------------------------------------------------------------------
// GameState
//
// Walks the Roblox pointer chain to cache frequently-needed instance pointers
// and exposes them to the embedded Lua VM via a "game" global table.
//
// Pointer chain:
//   module_base
//   + FakeDataModel::Pointer     → fake_dm
//   + FakeDataModel::RealDataModel → data_model
//   children scan for "Players" → players_svc
//   + Player::LocalPlayer        → local_player
//   + Player::ModelInstance      → character
//   children scan for "Humanoid" → humanoid
// ---------------------------------------------------------------------------
class XCUTOR_API GameState {
public:
    static GameState& get() {
        static GameState instance;
        return instance;
    }

    bool init(uintptr_t module_base);
    void shutdown();

    // Re-walk the pointer chain (call after respawn / teleport).
    void refresh_state() noexcept;

    // ── Cached pointers ───────────────────────────────────────────────────
    uintptr_t data_model()    const noexcept { return data_model_ptr;    }
    uintptr_t local_player()  const noexcept { return local_player_ptr;  }
    uintptr_t humanoid()      const noexcept { return humanoid_ptr;      }

    // ── Player reads ─────────────────────────────────────────────────────
    float    player_health()     const noexcept;
    float    player_max_health() const noexcept;
    float    player_walkspeed()  const noexcept;
    float    player_jumppower()  const noexcept;
    bool     player_is_alive()   const noexcept;
    int64_t  player_userid()     const noexcept;
    std::string player_display_name() const noexcept;

    // ── Player writes ─────────────────────────────────────────────────────
    void set_player_health(float v)    noexcept;
    void set_player_walkspeed(float v) noexcept;
    void set_player_jumppower(float v) noexcept;
    void set_player_jump(bool v)       noexcept;

    // ── Lua binding ───────────────────────────────────────────────────────
    void bind_to_lua(lua_State* L);

private:
    GameState() = default;

    uintptr_t module_base_      = 0;
    uintptr_t data_model_ptr    = 0;
    uintptr_t local_player_ptr  = 0;
    uintptr_t character_ptr     = 0;
    uintptr_t humanoid_ptr      = 0;
    bool      initialised_      = false;

    // ── Instance helpers ──────────────────────────────────────────────────
    static std::string   read_class_name(uintptr_t instance) noexcept;
    static std::string   read_rbx_string(uintptr_t str_addr) noexcept;
    static uintptr_t     find_child_by_class(uintptr_t instance,
                                             std::string_view class_name) noexcept;

    // ── Lua C-function implementations ────────────────────────────────────
    static int lua_get_health      (lua_State* L);
    static int lua_set_health      (lua_State* L);
    static int lua_get_max_health  (lua_State* L);
    static int lua_get_walkspeed   (lua_State* L);
    static int lua_set_walkspeed   (lua_State* L);
    static int lua_get_jumppower   (lua_State* L);
    static int lua_set_jumppower   (lua_State* L);
    static int lua_is_alive        (lua_State* L);
    static int lua_jump            (lua_State* L);
    static int lua_get_userid      (lua_State* L);
    static int lua_get_display_name(lua_State* L);
    static int lua_refresh         (lua_State* L);
};
