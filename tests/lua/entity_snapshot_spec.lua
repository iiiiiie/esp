local helper = require("test_helper")
local snapshot = require("core.entity_snapshot")

return {
    helper.case("field cells reject opaque values", function()
        local ok = pcall(function()
            snapshot.known(function() end, "test")
        end)
        helper.falsy(ok)
        helper.equal(snapshot.bridge("typed_provider").state, "bridge")
        helper.equal(snapshot.unavailable("missing_provider").state, "unavailable")
    end),

    helper.case("generation replacement scopes records", function()
        local store = snapshot.new_store()
        snapshot.reset_session(store, 3)
        local generation = snapshot.begin_generation(store, "reconcile", 12.5)
        local added, record = snapshot.add(generation, {
            kind = "pal",
            actor = "actor-a",
            fields = {
                level = snapshot.known(18, "test.level"),
            },
        })
        helper.truthy(added)
        helper.equal(record.session_id, 3)
        helper.equal(record.generation_id, 1)
        helper.equal(record.ordinal, 1)

        local duplicate = snapshot.add(generation, {
            kind = "pal",
            actor = "actor-a",
            fields = {},
        })
        helper.falsy(duplicate)
        snapshot.replace(store, generation)
        helper.equal(#snapshot.current(store).records, 1)

        local next_generation = snapshot.begin_generation(store, "reconcile", 18.0)
        snapshot.add(next_generation, {
            kind = "pal",
            actor = "actor-b",
            fields = {},
        })
        snapshot.replace(store, next_generation)
        helper.equal(snapshot.current(store).records[1].actor, "actor-b")
        helper.equal(snapshot.current(store).lookup["actor-a"], nil)
    end),

    helper.case("clear drops current actor lookup without access", function()
        local store = snapshot.new_store()
        snapshot.reset_session(store, 7)
        local generation = snapshot.begin_generation(store, "notify", 1)
        local actor = setmetatable({}, {
            __index = function()
                error("actor must not be dereferenced")
            end,
        })
        snapshot.add(generation, { kind = "pal", actor = actor, fields = {} })
        snapshot.replace(store, generation)
        snapshot.clear(store, "map_preload")
        helper.equal(#snapshot.current(store).records, 0)
    end),
}
