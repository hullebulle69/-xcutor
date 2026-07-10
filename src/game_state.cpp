#include "game_state.h"

// Luau C++ API (Luau is a C++ library — no extern "C" wrapper)
#include "lua.h"
#include "lualib.h"

#include <cstring>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------
bool GameState::init(uintptr_t base) {
    if (initialised) return true;
    module_base = base;
    refresh_player_ptr();
    initialised = true;
    return true;
}

void GameState::shutdown() {
    module_base       = 0;
    cached_player_ptr = 0;
    initialised       = false;
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
void GameState::refresh_player_ptr() noexcept {
    // The static pointer lives at module_base + offsets.static_player_ptr.
    // Dereference it to get the live object address.
    const uintptr_t ptr_addr = module_base + offsets.static_player_ptr;
    if (!ptr_addr) { cached_player_ptr = 0; return; }
    cached_player_ptr = mem::read<uintptr_t>(ptr_addr);
}

// ---------------------------------------------------------------------------
// Player entity reads
// ---------------------------------------------------------------------------
float GameState::player_health() const noexcept {
    if (!cached_player_ptr) return 0.f;
    return mem::read<float>(cached_player_ptr + offsets.health);
}

float GameState::player_max_health() const noexcept {
    if (!cached_player_ptr) return 0.f;
    return mem::read<float>(cached_player_ptr + offsets.max_health);
}

float GameState::player_stamina() const noexcept {
    if (!cached_player_ptr) return 0.f;
    return mem::read<float>(cached_player_ptr + offsets.stamina);
}

bool GameState::player_is_alive() const noexcept {
    return player_health() > 0.f;
}

// ---------------------------------------------------------------------------
// Player entity writes
// ---------------------------------------------------------------------------
void GameState::set_player_health(float v) noexcept {
    if (!cached_player_ptr) return;
    mem::write<float>(cached_player_ptr + offsets.health, v);
}

void GameState::set_player_stamina(float v) noexcept {
    if (!cached_player_ptr) return;
    mem::write<float>(cached_player_ptr + offsets.stamina, v);
}

// ---------------------------------------------------------------------------
// World state reads
// ---------------------------------------------------------------------------
int GameState::current_level() const noexcept {
    const uintptr_t addr = module_base + offsets.level;
    return addr ? mem::read<int>(addr) : 0;
}

double GameState::game_time() const noexcept {
    const uintptr_t addr = module_base + offsets.game_time;
    return addr ? mem::read<double>(addr) : 0.0;
}

bool GameState::is_paused() const noexcept {
    const uintptr_t addr = module_base + offsets.is_paused;
    return addr ? (mem::read<uint8_t>(addr) != 0) : false;
}

// ---------------------------------------------------------------------------
// Lua C-function implementations
//
// Convention used throughout:
//   • luaL_checkXxx() – mandatory argument; raises a Lua error on mismatch.
//   • luaL_optXxx()   – optional argument with a default.
//   • Returns the number of values pushed onto the Lua stack.
// ---------------------------------------------------------------------------

// Retrieve 'this' from registry key "GameState" (set in bind_to_lua).
static GameState& gs(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "GameState");
    auto* gs_ptr = static_cast<GameState*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return *gs_ptr;
}

int GameState::lua_get_health(lua_State* L) {
    lua_pushnumber(L, gs(L).player_health());
    return 1;
}

int GameState::lua_set_health(lua_State* L) {
    const float v = static_cast<float>(luaL_checknumber(L, 1));
    gs(L).set_player_health(v);
    return 0;
}

int GameState::lua_get_stamina(lua_State* L) {
    lua_pushnumber(L, gs(L).player_stamina());
    return 1;
}

int GameState::lua_set_stamina(lua_State* L) {
    const float v = static_cast<float>(luaL_checknumber(L, 1));
    gs(L).set_player_stamina(v);
    return 0;
}

int GameState::lua_is_alive(lua_State* L) {
    lua_pushboolean(L, gs(L).player_is_alive() ? 1 : 0);
    return 1;
}

int GameState::lua_get_level(lua_State* L) {
    lua_pushinteger(L, gs(L).current_level());
    return 1;
}

int GameState::lua_get_game_time(lua_State* L) {
    lua_pushnumber(L, gs(L).game_time());
    return 1;
}

int GameState::lua_is_paused(lua_State* L) {
    lua_pushboolean(L, gs(L).is_paused() ? 1 : 0);
    return 1;
}

// ---------------------------------------------------------------------------
// Lua binding – expose a "game" global table
// ---------------------------------------------------------------------------
void GameState::bind_to_lua(lua_State* L) {
    // Store 'this' in the registry so lua_get_* functions can reach it.
    lua_pushlightuserdata(L, this);
    lua_setfield(L, LUA_REGISTRYINDEX, "GameState");

    // Build the "game" table.
    lua_newtable(L);

    // Player reads / writes.
    static const struct { const char* name; lua_CFunction fn; } fns[] = {
        { "getHealth",   lua_get_health   },
        { "setHealth",   lua_set_health   },
        { "getStamina",  lua_get_stamina  },
        { "setStamina",  lua_set_stamina  },
        { "isAlive",     lua_is_alive     },

        // World reads.
        { "getLevel",    lua_get_level    },
        { "getTime",     lua_get_game_time },
        { "isPaused",    lua_is_paused    },
    };

    for (const auto& f : fns) {
        lua_pushcfunction(L, f.fn, f.name);
        lua_setfield(L, -2, f.name);
    }

    // Attach the table as global "game".
    lua_setglobal(L, "game");
}
