local adapter_registry = {}

local registry_methods = {}
registry_methods.__index = registry_methods

local function require_function(value, name)
    assert(type(value) == "function", name .. " must be a function")
end

function adapter_registry.new()
    return setmetatable({
        ordered = {},
        by_id = {},
    }, registry_methods)
end

function registry_methods:register(descriptor)
    assert(type(descriptor) == "table", "adapter descriptor must be a table")
    assert(type(descriptor.id) == "string" and descriptor.id ~= "", "adapter id is required")
    assert(self.by_id[descriptor.id] == nil, "adapter already registered: " .. descriptor.id)
    require_function(descriptor.classify, "adapter classify")
    require_function(descriptor.normalize, "adapter normalize")

    self.by_id[descriptor.id] = descriptor
    self.ordered[#self.ordered + 1] = descriptor
    return descriptor
end

function registry_methods:get(id)
    return self.by_id[id]
end

function registry_methods:list()
    local result = {}
    for index, descriptor in ipairs(self.ordered) do
        result[index] = descriptor
    end
    return result
end

function registry_methods:admit(runtime, actor, context)
    assert(type(runtime) == "table", "runtime must be a table")
    require_function(runtime.reject_player_representation, "runtime player gate")

    local player_reason = runtime.reject_player_representation(actor)
    if player_reason ~= nil then
        return nil, {
            stage = "player_gate",
            reason = player_reason,
        }
    end

    for _, descriptor in ipairs(self.ordered) do
        local classified = descriptor.classify(runtime, actor, context)
        if classified ~= nil then
            if classified.accepted ~= true then
                return nil, {
                    stage = "adapter",
                    adapter_id = descriptor.id,
                    reason = classified.reason or "adapter_rejected",
                }
            end

            local fields = descriptor.normalize(runtime, classified, context)
            if type(fields) ~= "table" then
                return nil, {
                    stage = "adapter",
                    adapter_id = descriptor.id,
                    reason = "normalization_unavailable",
                }
            end

            return {
                kind = descriptor.kind or descriptor.id,
                actor = actor,
                source = context and context.source or "unknown",
                accepted_at = context and context.accepted_at or 0,
                fields = fields,
            }, nil
        end
    end

    return nil, {
        stage = "adapter",
        reason = "unknown_type",
    }
end

return adapter_registry
