local helper = require("test_helper")
local registry_module = require("core.adapter_registry")

return {
    helper.case("player gate runs before every adapter", function()
        local trace = {}
        local registry = registry_module.new()
        registry:register({
            id = "fake",
            classify = function()
                trace[#trace + 1] = "adapter"
                return { accepted = true }
            end,
            normalize = function()
                return {}
            end,
        })

        local entity, rejection = registry:admit({
            reject_player_representation = function()
                trace[#trace + 1] = "player_gate"
                return "player_class"
            end,
        }, "player", {})
        helper.equal(entity, nil)
        helper.equal(rejection.stage, "player_gate")
        helper.equal(#trace, 1)
        helper.equal(trace[1], "player_gate")
    end),

    helper.case("a second adapter needs no core changes", function()
        local registry = registry_module.new()
        registry:register({
            id = "fake_resource",
            kind = "resource",
            classify = function(_, actor)
                if actor ~= "ore" then
                    return nil
                end
                return { accepted = true }
            end,
            normalize = function()
                return {}
            end,
        })
        local entity = registry:admit({
            reject_player_representation = function()
                return nil
            end,
        }, "ore", { source = "test", accepted_at = 5 })
        helper.equal(entity.kind, "resource")
        helper.equal(entity.source, "test")
    end),
}
