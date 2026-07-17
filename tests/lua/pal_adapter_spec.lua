local helper = require("test_helper")
local pal_adapter = require("adapters.pal")

return {
    helper.case("Pal normalization retains only safe field cells", function()
        local descriptor = pal_adapter.new()
        local actor = {}
        local fields = descriptor.normalize({
            probe_pal_fields = function()
                return {
                    level = { available = true, value = 18, path = "parameter.method:GetLevel" },
                    species = { available = true, value = actor, path = "parameter.method:GetCharacterID" },
                }
            end,
            distance_m = function()
                return 245.2
            end,
        }, {
            accepted = true,
        }, {
            actor = actor,
        })

        helper.equal(fields.level.state, "known")
        helper.equal(fields.level.value, 18)
        helper.equal(fields.distance_m.state, "known")
        helper.equal(fields.species.state, "bridge")
        helper.equal(fields.species.value, nil)
        helper.equal(fields.passive_skills.state, "unavailable")
        helper.equal(fields.elements.state, "bridge")
        helper.equal(fields.elements.reason, "blueprint_element_mask_adapter")
    end),
}
