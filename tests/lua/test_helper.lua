local helper = {}

function helper.case(name, run)
    return {
        name = name,
        run = run,
    }
end

function helper.equal(actual, expected, message)
    if actual ~= expected then
        error(message or string.format("expected %s, got %s", tostring(expected), tostring(actual)), 2)
    end
end

function helper.truthy(value, message)
    if not value then
        error(message or "expected a truthy value", 2)
    end
end

function helper.falsy(value, message)
    if value then
        error(message or "expected a falsy value", 2)
    end
end

return helper
