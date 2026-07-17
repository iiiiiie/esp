import fs from "node:fs";
import path from "node:path";
import process from "node:process";
import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";
import { lua, lauxlib, lualib, to_luastring, to_jsstring } from "fengari";
import luaparse from "luaparse";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const runtimeRoot = path.join(root, "PalworldResourceESP", "Scripts");
const testRoot = path.join(root, "tests", "lua");

function luaFiles(directory) {
  return fs.readdirSync(directory, { withFileTypes: true }).flatMap((entry) => {
    const fullPath = path.join(directory, entry.name);
    return entry.isDirectory() ? luaFiles(fullPath) : entry.name.endsWith(".lua") ? [fullPath] : [];
  });
}

const files = [...luaFiles(runtimeRoot), ...luaFiles(testRoot)];
for (const file of files) {
  luaparse.parse(fs.readFileSync(file, "utf8"), { luaVersion: "5.3" });
}

const forbiddenRuntimeGlobals = /\b(?:FindAllOf|FindFirstOf|StaticFindObject|RegisterHook|NotifyOnNewObject)\b/;
for (const file of luaFiles(path.join(runtimeRoot, "core"))) {
  const source = fs.readFileSync(file, "utf8");
  if (forbiddenRuntimeGlobals.test(source)) {
    throw new Error(`Pure core imports a game runtime global: ${path.relative(root, file)}`);
  }
}

const normalize = (value) => value.replaceAll("\\", "/");
const packagePaths = [
  `${normalize(runtimeRoot)}/?.lua`,
  `${normalize(runtimeRoot)}/?/init.lua`,
  `${normalize(testRoot)}/?.lua`,
].join(";");
const entrypoint = normalize(path.join(testRoot, "run.lua"));
const runtimeEntrypoint = normalize(path.join(runtimeRoot, "main.lua"));
const source = `
package.path = [[${packagePaths}]] .. ";" .. package.path
dofile([[${entrypoint}]])

local runtime_logs = {}
local original_print = print
print = function(message)
    runtime_logs[#runtime_logs + 1] = tostring(message)
    original_print(message)
end

local fake_time = 1000
os.time = function()
    return fake_time
end

local classes = {}
StaticFindObject = function(path)
    classes[path] = classes[path] or { path = path }
    return classes[path]
end
RegisterCustomEvent = function() end
local bridge_begin_play_hook = nil
RegisterBeginPlayPostHook = function(callback)
    bridge_begin_play_hook = callback
    return 1
end
local monster_notification = nil
NotifyOnNewObject = function(_, callback)
    monster_notification = callback
end
RegisterHook = function() return 1, 2 end
local panel_keybind = nil
Key = { E = 69 }
ModifierKey = { SHIFT = 1 }
RegisterKeyBind = function(_, _, callback)
    panel_keybind = callback
end
local load_map_pre_hook = nil
RegisterLoadMapPreHook = function(callback)
    load_map_pre_hook = callback
end
RegisterLoadMapPostHook = function() end

local reconcile_loop = nil
local delayed_callbacks = {}
LoopInGameThreadWithDelay = function(_, callback)
    reconcile_loop = callback
    return true
end
ExecuteInGameThreadWithDelay = function(_, callback)
    delayed_callbacks[#delayed_callbacks + 1] = callback
end

local player = {}
function player:IsA(class_object)
    return class_object.path == "/Script/Pal.PalPlayerCharacter"
end

local camera_manager = {}
function camera_manager:GetCameraLocation()
    return { X = 0, Y = 0, Z = 0 }
end
local controller = { PlayerCameraManager = camera_manager }
FindFirstOf = function()
    return controller
end

local monsters = {}
for index = 1, 4 do
    local component = { Trainer = nil, NPCSpawnedOtomoTrainer = nil }
    function component:IsDead()
        return false
    end

    local parameter = {
        SaveParameter = {
            IsPlayer = false,
            OwnerPlayerUId = { A = 0, B = 0, C = 0, D = 0 },
        },
    }
    function parameter:GetLevel()
        return index
    end

    local actor = {}
    function actor:IsA(class_object)
        return class_object.path == "/Script/Pal.PalMonsterCharacter"
    end
    function actor:GetCharacterParameterComponent()
        return component
    end
    function actor:GetIndividualParameter()
        return parameter
    end
    function actor:K2_GetActorLocation()
        return { X = index * 100, Y = 0, Z = 0 }
    end
    monsters[#monsters + 1] = actor
end

local event_component = { Trainer = nil, NPCSpawnedOtomoTrainer = nil }
function event_component:IsDead()
    return false
end
local event_parameter = {
    SaveParameter = {
        IsPlayer = false,
        OwnerPlayerUId = { A = 0, B = 0, C = 0, D = 0 },
    },
}
function event_parameter:GetLevel()
    return 5
end
local event_actor = {}
function event_actor:IsA(class_object)
    return class_object.path == "/Script/Pal.PalMonsterCharacter"
end
function event_actor:GetCharacterParameterComponent()
    return event_component
end
function event_actor:GetIndividualParameter()
    return event_parameter
end
function event_actor:K2_GetActorLocation()
    return { X = 500, Y = 0, Z = 0 }
end

local bridge_class = {}
function bridge_class:GetFullName()
    return "BlueprintGeneratedClass /Game/Mods/PalworldResourceESP/ModActor.ModActor_C"
end
local panel_toggle_count = 0
local bridge_actor = {
    ESP_ControlRevision = 0,
    ESP_RuntimeEnabled = true,
    ESP_ProfileId = 2,
    ESP_PresetId = 1,
    ESP_CaptureRequested = false,
    ESP_BridgeGenderDiagnosticCode = 0,
}
function bridge_actor:GetClass()
    return bridge_class
end
function bridge_actor:PalworldResourceESP_ResetSession() end
function bridge_actor:PalworldResourceESP_SetTarget() end
function bridge_actor:PalworldResourceESP_ClearTarget() end
function bridge_actor:PalworldResourceESP_TogglePanel()
    panel_toggle_count = panel_toggle_count + 1
end

FindAllOf = function(class_name)
    if class_name == "PalPlayerCharacter" then
        return { player }
    end
    if class_name == "PalMonsterCharacter" then
        return monsters
    end
    return {}
end

dofile([[${runtimeEntrypoint}]])
print("Runtime entrypoint load passed")

assert(type(reconcile_loop) == "function", "reconcile loop was not captured")
delayed_callbacks = {}
reconcile_loop()
assert(#delayed_callbacks == 1, "first reconcile batch was not scheduled")

local executed_callbacks = 0
while #delayed_callbacks > 0 do
    executed_callbacks = executed_callbacks + 1
    assert(executed_callbacks <= 10, "reconcile batch queue did not terminate")
    local callback = table.remove(delayed_callbacks, 1)
    callback()
end
assert(executed_callbacks == 2, "expected two reconcile batches")

local found_chunked_result = false
for _, message in ipairs(runtime_logs) do
    if message:match("SCAN_DONE.*admitted=4.*batches=2") then
        found_chunked_result = true
        break
    end
end
assert(found_chunked_result, "chunked reconcile result was not logged")
print("Chunked reconcile stub passed")

assert(type(bridge_begin_play_hook) == "function", "bridge discovery hook was not captured")
assert(type(panel_keybind) == "function", "Shift+E keybind was not captured")
bridge_begin_play_hook(bridge_actor)
panel_keybind()
assert(panel_toggle_count == 1, "Shift+E did not call the panel toggle bridge")

bridge_actor.ESP_CaptureRequested = true
bridge_actor.ESP_ControlRevision = 1
reconcile_loop()
bridge_actor.ESP_RuntimeEnabled = false
bridge_actor.ESP_ControlRevision = 2
reconcile_loop()
assert(#delayed_callbacks == 0, "runtime-off mode scheduled discovery work")
assert(type(monster_notification) == "function", "monster notification was not captured")
monster_notification(event_actor)
assert(#delayed_callbacks == 0, "runtime-off notification scheduled admission work")

bridge_actor.ESP_CaptureRequested = false
bridge_actor.ESP_ControlRevision = 3
reconcile_loop()
bridge_actor.ESP_RuntimeEnabled = true
bridge_actor.ESP_ProfileId = 3
bridge_actor.ESP_ControlRevision = 4
reconcile_loop()
while #delayed_callbacks > 0 do
    local callback = table.remove(delayed_callbacks, 1)
    callback()
end
monster_notification(event_actor)
assert(#delayed_callbacks == 1, "event-first notification did not schedule one queued admission")
table.remove(delayed_callbacks, 1)()

local capture_start_found = false
local capture_mode_found = false
local capture_stop_found = false
local event_admission_found = false
for _, message in ipairs(runtime_logs) do
    capture_start_found = capture_start_found or message:match("PERF_SESSION_START") ~= nil
    capture_mode_found = capture_mode_found or message:match("PERF_MODE_CHANGED.*profile=off") ~= nil
    capture_stop_found = capture_stop_found or message:match("PERF_SESSION_STOP") ~= nil
    event_admission_found = event_admission_found or message:match("ENTITY_SNAPSHOT.*source=notify_queue.*admitted=5") ~= nil
end
assert(capture_start_found and capture_mode_found and capture_stop_found, "capture markers were incomplete")
assert(event_admission_found, "event-first queued actor was not admitted")
print("Panel controls and event-first queue passed")

assert(type(load_map_pre_hook) == "function", "load-map pre-hook was not captured")
bridge_actor.ESP_ProfileId = 2
bridge_actor.ESP_ControlRevision = 5
reconcile_loop()
while #delayed_callbacks > 0 do
    local callback = table.remove(delayed_callbacks, 1)
    callback()
end
fake_time = fake_time + 6
reconcile_loop()
assert(#delayed_callbacks == 1, "cancellable reconcile batch was not scheduled")
local scan_done_count_before_cancel = 0
for _, message in ipairs(runtime_logs) do
    if message:match("SCAN_DONE") then
        scan_done_count_before_cancel = scan_done_count_before_cancel + 1
    end
end
load_map_pre_hook()
local stale_callback = table.remove(delayed_callbacks, 1)
stale_callback()

local scan_done_count_after_cancel = 0
for _, message in ipairs(runtime_logs) do
    if message:match("SCAN_DONE") then
        scan_done_count_after_cancel = scan_done_count_after_cancel + 1
    end
end
assert(scan_done_count_after_cancel == scan_done_count_before_cancel, "cancelled reconcile committed a stale generation")
print("Chunked reconcile cancellation passed")
`;

const L = lauxlib.luaL_newstate();
lualib.luaL_openlibs(L);
let status = lauxlib.luaL_loadstring(L, to_luastring(source));
if (status === lua.LUA_OK) {
  status = lua.lua_pcall(L, 0, lua.LUA_MULTRET, 0);
}
if (status !== lua.LUA_OK) {
  const message = to_jsstring(lua.lua_tostring(L, -1));
  throw new Error(message);
}

console.log(`Parsed Lua files: ${files.length}`);
console.log("Pure core runtime-global check passed");

if (process.platform === "win32") {
  const performanceTests = path.join(root, "tests", "performance", "run-performance-tests.ps1");
  const result = spawnSync(
    "powershell.exe",
    ["-NoProfile", "-ExecutionPolicy", "Bypass", "-File", performanceTests],
    { stdio: "inherit" },
  );
  if (result.error) {
    throw result.error;
  }
  if (result.status !== 0) {
    throw new Error(`PowerShell performance tests exited with code ${result.status}`);
  }
}

process.exitCode = 0;
