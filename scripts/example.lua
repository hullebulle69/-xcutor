-- example.lua
-- Demonstrates the xcutor Lua API surface.

-- ── Query game state ──────────────────────────────────────────────────────
local hp      = game.getHealth()
local stamina = game.getStamina()
local level   = game.getLevel()
local t       = game.getTime()

print(string.format(
    "Player | hp=%.1f  stamina=%.1f  level=%d  time=%.2f",
    hp, stamina, level, t
))

-- ── Conditional logic ─────────────────────────────────────────────────────
if game.isAlive() then
    if hp < 20 then
        print("Low HP – restoring to 100")
        game.setHealth(100)
    end

    if stamina < 10 then
        print("Low stamina – restoring to 50")
        game.setStamina(50)
    end
else
    print("Player is dead – cannot restore stats")
end

-- ── Pause detection ───────────────────────────────────────────────────────
if game.isPaused() then
    print("Game is paused")
end

-- ── Reading a host-injected global ────────────────────────────────────────
-- The C++ side can call xcutor_set_global_string("config", "hard") before
-- running this script to drive different behaviour from outside.
local config = config or "normal"
print("Running in config:", config)
