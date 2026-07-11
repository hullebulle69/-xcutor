#include "game_state.h"

// Luau C++ API (Luau is a C++ library — no extern "C" wrapper)
#include "lua.h"
#include "lualib.h"

#include <cstring>

// ---------------------------------------------------------------------------
// Instance helpers
// ---------------------------------------------------------------------------

// Read the class name of a Roblox instance via ClassDescriptor.
// Class names are short ASCII strings stored as a C-string pointer at
// ClassDescriptor + Instance::ClassName.
std::string GameState::read_class_name(uintptr_t instance) noexcept {
    if (!instance) return {};
    const uintptr_t desc = mem::read<uintptr_t>(instance + offsets::Instance::ClassDescriptor);
    if (!desc) return {};
    const uintptr_t name_ptr = mem::read<uintptr_t>(desc + offsets::Instance::ClassName);
    if (!name_ptr) return {};
    char buf[64] = {};
    for (int i = 0; i < 63; ++i) {
        const char c = mem::read<char>(name_ptr + static_cast<uintptr_t>(i));
        if (!c) break;
        buf[i] = c;
    }
    return buf;
}

// Read a Roblox std::string (64-bit layout):
//   offset 0x00 : char* data  (pointer when length >= 16, inline otherwise)
//   offset 0x10 : uint32_t length  (Misc::StringLength)
std::string GameState::read_rbx_string(uintptr_t addr) noexcept {
    if (!addr) return {};
    const uint32_t length = mem::read<uint32_t>(addr + offsets::Misc::StringLength);
    if (length == 0 || length > 4096) return {};
    uintptr_t data_ptr;
    if (length < 16) {
        data_ptr = addr;               // inline storage
    } else {
        data_ptr = mem::read<uintptr_t>(addr);
        if (!data_ptr) return {};
    }
    std::string result(length, '\0');
    for (uint32_t i = 0; i < length; ++i)
        result[i] = mem::read<char>(data_ptr + i);
    return result;
}

// Walk an instance's children vector and return the first child whose class
// name matches target_class.  Returns 0 if not found.
uintptr_t GameState::find_child_by_class(uintptr_t instance,
                                         std::string_view target_class) noexcept {
    if (!instance) return 0;
    // Children are stored as a std::vector-style structure:
    //   instance + ChildrenStart           → begin pointer (pointer to child ptrs)
    //   instance + ChildrenStart + ChildrenEnd → end pointer
    const uintptr_t begin = mem::read<uintptr_t>(
        instance + offsets::Instance::ChildrenStart);
    const uintptr_t end   = mem::read<uintptr_t>(
        instance + offsets::Instance::ChildrenStart + offsets::Instance::ChildrenEnd);

    if (!begin || begin >= end) return 0;

    for (uintptr_t p = begin; p < end; p += sizeof(uintptr_t)) {
        const uintptr_t child = mem::read<uintptr_t>(p);
        if (!child) continue;
        if (read_class_name(child) == target_class) return child;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool GameState::init(uintptr_t base) {
    if (initialised_) return true;
    module_base_ = base;
    refresh_state();
    initialised_ = true;
    return true;
}

void GameState::shutdown() {
    module_base_     = 0;
    data_model_ptr   = 0;
    local_player_ptr = 0;
    character_ptr    = 0;
    humanoid_ptr     = 0;
    initialised_     = false;
}

// ---------------------------------------------------------------------------
// Pointer chain walk
// ---------------------------------------------------------------------------

void GameState::refresh_state() noexcept {
    data_model_ptr = local_player_ptr = character_ptr = humanoid_ptr = 0;

    // FakeDataModel static pointer → RealDataModel
    const uintptr_t fake_dm = mem::read<uintptr_t>(
        module_base_ + offsets::FakeDataModel::Pointer);
    if (!fake_dm) return;
    data_model_ptr = mem::read<uintptr_t>(
        fake_dm + offsets::FakeDataModel::RealDataModel);
    if (!data_model_ptr) return;

    // DataModel children → "Players" service
    const uintptr_t players_svc = find_child_by_class(data_model_ptr, "Players");
    if (!players_svc) return;

    // Players::LocalPlayer
    local_player_ptr = mem::read<uintptr_t>(
        players_svc + offsets::Player::LocalPlayer);
    if (!local_player_ptr) return;

    // LocalPlayer::ModelInstance → Character
    character_ptr = mem::read<uintptr_t>(
        local_player_ptr + offsets::Player::ModelInstance);
    if (!character_ptr) return;

    // Character children → "Humanoid"
    humanoid_ptr = find_child_by_class(character_ptr, "Humanoid");
}

// ---------------------------------------------------------------------------
// Player reads
// ---------------------------------------------------------------------------

float GameState::player_health() const noexcept {
    if (!humanoid_ptr) return 0.f;
    return mem::read<float>(humanoid_ptr + offsets::Humanoid::Health);
}

float GameState::player_max_health() const noexcept {
    if (!humanoid_ptr) return 0.f;
    return mem::read<float>(humanoid_ptr + offsets::Humanoid::MaxHealth);
}

float GameState::player_walkspeed() const noexcept {
    if (!humanoid_ptr) return 0.f;
    return mem::read<float>(humanoid_ptr + offsets::Humanoid::Walkspeed);
}

float GameState::player_jumppower() const noexcept {
    if (!humanoid_ptr) return 0.f;
    return mem::read<float>(humanoid_ptr + offsets::Humanoid::JumpPower);
}

bool GameState::player_is_alive() const noexcept {
    return player_health() > 0.f;
}

int64_t GameState::player_userid() const noexcept {
    if (!local_player_ptr) return 0;
    return mem::read<int64_t>(local_player_ptr + offsets::Player::UserId);
}

std::string GameState::player_display_name() const noexcept {
    if (!local_player_ptr) return {};
    return read_rbx_string(local_player_ptr + offsets::Player::DisplayName);
}

// ---------------------------------------------------------------------------
// Player writes
// ---------------------------------------------------------------------------

void GameState::set_player_health(float v) noexcept {
    if (!humanoid_ptr) return;
    mem::write<float>(humanoid_ptr + offsets::Humanoid::Health, v);
}

void GameState::set_player_walkspeed(float v) noexcept {
    if (!humanoid_ptr) return;
    mem::write<float>(humanoid_ptr + offsets::Humanoid::Walkspeed, v);
    // Also write the walkspeed check field to bypass the 16 wu/s sanity check.
    mem::write<float>(humanoid_ptr + offsets::Humanoid::WalkspeedCheck, v);
}

void GameState::set_player_jumppower(float v) noexcept {
    if (!humanoid_ptr) return;
    mem::write<float>(humanoid_ptr + offsets::Humanoid::JumpPower, v);
}

void GameState::set_player_jump(bool v) noexcept {
    if (!humanoid_ptr) return;
    mem::write<bool>(humanoid_ptr + offsets::Humanoid::Jump, v);
}

// ---------------------------------------------------------------------------
// Lua C-function implementations
//
// 'this' is stashed in the Lua registry under "GameState".
// ---------------------------------------------------------------------------
static GameState& gs(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "GameState");
    auto* ptr = static_cast<GameState*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return *ptr;
}

int GameState::lua_get_health(lua_State* L) {
    lua_pushnumber(L, gs(L).player_health());
    return 1;
}
int GameState::lua_set_health(lua_State* L) {
    gs(L).set_player_health(static_cast<float>(luaL_checknumber(L, 1)));
    return 0;
}
int GameState::lua_get_max_health(lua_State* L) {
    lua_pushnumber(L, gs(L).player_max_health());
    return 1;
}
int GameState::lua_get_walkspeed(lua_State* L) {
    lua_pushnumber(L, gs(L).player_walkspeed());
    return 1;
}
int GameState::lua_set_walkspeed(lua_State* L) {
    gs(L).set_player_walkspeed(static_cast<float>(luaL_checknumber(L, 1)));
    return 0;
}
int GameState::lua_get_jumppower(lua_State* L) {
    lua_pushnumber(L, gs(L).player_jumppower());
    return 1;
}
int GameState::lua_set_jumppower(lua_State* L) {
    gs(L).set_player_jumppower(static_cast<float>(luaL_checknumber(L, 1)));
    return 0;
}
int GameState::lua_is_alive(lua_State* L) {
    lua_pushboolean(L, gs(L).player_is_alive() ? 1 : 0);
    return 1;
}
int GameState::lua_jump(lua_State* L) {
    gs(L).set_player_jump(true);
    return 0;
}
int GameState::lua_get_userid(lua_State* L) {
    lua_pushinteger(L, static_cast<lua_Integer>(gs(L).player_userid()));
    return 1;
}
int GameState::lua_get_display_name(lua_State* L) {
    const std::string name = gs(L).player_display_name();
    lua_pushlstring(L, name.c_str(), name.size());
    return 1;
}
int GameState::lua_refresh(lua_State* L) {
    gs(L).refresh_state();
    return 0;
}

// ---------------------------------------------------------------------------
// Lua binding — "game" global table
// ---------------------------------------------------------------------------
void GameState::bind_to_lua(lua_State* L) {
    lua_pushlightuserdata(L, this);
    lua_setfield(L, LUA_REGISTRYINDEX, "GameState");

    lua_newtable(L);

    static const struct { const char* name; lua_CFunction fn; } fns[] = {
        { "getHealth",      lua_get_health       },
        { "setHealth",      lua_set_health       },
        { "getMaxHealth",   lua_get_max_health   },
        { "getWalkspeed",   lua_get_walkspeed    },
        { "setWalkspeed",   lua_set_walkspeed    },
        { "getJumppower",   lua_get_jumppower    },
        { "setJumppower",   lua_set_jumppower    },
        { "isAlive",        lua_is_alive         },
        { "jump",           lua_jump             },
        { "getUserId",      lua_get_userid       },
        { "getDisplayName", lua_get_display_name },
        { "refresh",        lua_refresh          },
    };
    for (const auto& f : fns) {
        lua_pushcfunction(L, f.fn, f.name);
        lua_setfield(L, -2, f.name);
    }

    lua_setglobal(L, "game");
}
