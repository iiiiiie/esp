local specs = {
    "entity_snapshot_spec",
    "filter_engine_spec",
    "adapter_registry_spec",
    "pal_adapter_spec",
}

local passed = 0
for _, spec_name in ipairs(specs) do
    local cases = require(spec_name)
    for _, test_case in ipairs(cases) do
        local ok, err = pcall(test_case.run)
        if not ok then
            error(string.format("FAIL %s: %s", test_case.name, tostring(err)))
        end
        passed = passed + 1
        print("PASS " .. test_case.name)
    end
end

assert(passed > 0, "no Lua assertions executed")
print(string.format("Lua tests passed: %d", passed))
