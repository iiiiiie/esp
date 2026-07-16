import fs from "node:fs";
import path from "node:path";
import process from "node:process";
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

local classes = {}
StaticFindObject = function(path)
    classes[path] = classes[path] or { path = path }
    return classes[path]
end
RegisterCustomEvent = function() end
RegisterBeginPlayPostHook = function() return 1 end
NotifyOnNewObject = function() end
RegisterHook = function() return 1, 2 end
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

assert(type(load_map_pre_hook) == "function", "load-map pre-hook was not captured")
reconcile_loop()
assert(#delayed_callbacks == 1, "cancellable reconcile batch was not scheduled")
load_map_pre_hook()
local stale_callback = table.remove(delayed_callbacks, 1)
stale_callback()

local scan_done_count = 0
for _, message in ipairs(runtime_logs) do
    if message:match("SCAN_DONE") then
        scan_done_count = scan_done_count + 1
    end
end
assert(scan_done_count == 1, "cancelled reconcile committed a stale generation")
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
process.exitCode = 0;
