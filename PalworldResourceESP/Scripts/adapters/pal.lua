local entity_snapshot = require("core.entity_snapshot")

local pal_adapter = {}

local function is_finite_number(value)
    return type(value) == "number" and value == value and value ~= math.huge and value ~= -math.huge
end

local function normalized_level(raw_fields)
    local level = raw_fields and raw_fields.level
    if type(level) == "table" and level.available == true and is_finite_number(level.value) then
        return entity_snapshot.known(level.value, level.path or "parameter.method:GetLevel")
    end
    return entity_snapshot.unavailable("lua_safe_scalar_not_available")
end

local function normalized_distance(runtime, actor, context)
    if type(runtime.distance_m) ~= "function" then
        return entity_snapshot.unavailable("distance_provider_not_registered")
    end
    local distance = runtime.distance_m(actor, context)
    if is_finite_number(distance) and distance >= 0 then
        return entity_snapshot.known(distance, "camera_to_actor")
    end
    return entity_snapshot.unavailable("camera_or_actor_location_unavailable")
end

function pal_adapter.new()
    return {
        id = "pal",
        kind = "pal",
        notification_class = "/Script/Pal.PalMonsterCharacter",
        find_all = function(runtime)
            return runtime.find_all("PalMonsterCharacter")
        end,
        classify = function(runtime, actor)
            local classification, component, parameter, save_parameter = runtime.classify_pal(actor)
            if classification == "unknown_type" then
                return nil
            end
            if classification ~= "wild" then
                return {
                    accepted = false,
                    reason = classification,
                }
            end
            return {
                accepted = true,
                actor = actor,
                component = component,
                parameter = parameter,
                save_parameter = save_parameter,
            }
        end,
        normalize = function(runtime, classified, context)
            local raw_fields = nil
            if type(runtime.probe_pal_fields) == "function" then
                raw_fields = runtime.probe_pal_fields(
                    classified.actor,
                    classified.component,
                    classified.parameter,
                    classified.save_parameter
                )
            end

            return {
                is_wild = entity_snapshot.known(true, "pal_adapter.classification"),
                level = normalized_level(raw_fields),
                distance_m = normalized_distance(runtime, classified.actor, context),
                species = entity_snapshot.bridge("fname_userdata"),
                gender = entity_snapshot.bridge("blueprint_enum_adapter"),
                passive_skills = entity_snapshot.unavailable("adapter_not_proven"),
                iv_hp = entity_snapshot.bridge("blueprint_save_parameter_talent_adapter"),
                iv_attack = entity_snapshot.bridge("blueprint_save_parameter_talent_adapter"),
                iv_melee = entity_snapshot.unavailable("game_build_field_not_confirmed"),
                iv_defense = entity_snapshot.bridge("blueprint_save_parameter_talent_adapter"),
                lucky = entity_snapshot.unavailable("adapter_not_proven"),
                alpha_boss = entity_snapshot.unavailable("adapter_not_proven"),
                elements = entity_snapshot.bridge("blueprint_element_mask_adapter"),
                capture_count = entity_snapshot.unavailable("local_collection_adapter_not_proven"),
            }
        end,
    }
end

return pal_adapter
