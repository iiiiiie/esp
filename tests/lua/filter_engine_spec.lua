local helper = require("test_helper")
local snapshot = require("core.entity_snapshot")
local filters = require("core.filter_engine")

local function record(ordinal, distance, fields)
    fields = fields or {}
    fields.distance_m = distance and snapshot.known(distance, "test.distance")
        or snapshot.unavailable("test.distance_missing")
    return {
        kind = "pal",
        ordinal = ordinal,
        fields = fields,
    }
end

return {
    helper.case("exact range and boolean filters compose with AND", function()
        local candidate = record(1, 25, {
            level = snapshot.known(20, "test.level"),
            lucky = snapshot.known(true, "test.lucky"),
            species = snapshot.known("Lamball", "test.species"),
        })
        local matched = filters.match(candidate, {
            kinds = { "pal" },
            fields = {
                level = { mode = "range", min = 10, max = 30 },
                lucky = { mode = "boolean", value = true },
                species = { mode = "exact", values = { "Lamball", "Cattiva" } },
            },
        })
        helper.truthy(matched)
    end),

    helper.case("bridge and unavailable fields fail closed", function()
        local candidate = record(1, 25, {
            gender = snapshot.bridge("typed_provider"),
        })
        local matched, reason = filters.match(candidate, {
            fields = {
                gender = { mode = "exact", value = "female" },
            },
        })
        helper.falsy(matched)
        helper.equal(reason, "gender:field_not_known")
    end),

    helper.case("list any all and exclude modes are explicit", function()
        local candidate = record(1, 25, {
            elements = snapshot.known({ "fire", "dragon" }, "test.elements"),
        })
        helper.truthy(filters.match(candidate, {
            fields = { elements = { mode = "any", values = { "water", "fire" } } },
        }))
        helper.truthy(filters.match(candidate, {
            fields = { elements = { mode = "all", values = { "fire", "dragon" } } },
        }))
        helper.truthy(filters.match(candidate, {
            fields = { elements = { mode = "exclude", values = { "ice" } } },
        }))
        helper.falsy(filters.match(candidate, {
            fields = { elements = { mode = "exclude", values = { "dragon" } } },
        }))
    end),

    helper.case("ordering and display budget are deterministic", function()
        local ordered = filters.order({
            record(1, nil),
            record(2, 90),
            record(3, 10),
            record(4, 10),
        })
        helper.equal(ordered[1].ordinal, 3)
        helper.equal(ordered[2].ordinal, 4)
        helper.equal(ordered[3].ordinal, 2)
        helper.equal(ordered[4].ordinal, 1)

        local displayed, truncated = filters.budget(ordered, 2)
        helper.equal(#displayed, 2)
        helper.equal(truncated, 2)
    end),
}
