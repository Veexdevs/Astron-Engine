-- Astron Engine — Módulo Nativo de Comportamentos & Cutscenes em Lua 5.4
-- Executado pelo subsistema de scripting rápido ao lado dos scripts C# (.cs).

local AstronBehaviors = {
    engine = "Astron Engine",
    version = "1.0.0",
    registered = {}
}

--- Registra um comportamento reativo para entidades da cena 3D.
---@param name string Nome do comportamento (ex: "RotatingCoin", "MovingPlatform", "Checkpoint")
---@param callbacks table Tabela com hooks onStart(entity) e onUpdate(entity, dt, time)
function AstronBehaviors.register(name, callbacks)
    assert(type(name) == "string" and #name > 0, "Nome do comportamento inválido")
    assert(type(callbacks) == "table", "Callbacks devem ser uma tabela Lua")
    AstronBehaviors.registered[name] = callbacks
end

-- Plataforma oscilante para fases de Obby 3D
AstronBehaviors.register("MovingPlatform", {
    onStart = function(entity)
        entity.originX = entity.position.x
        entity.amplitude = entity.amplitude or 4.5
        entity.speed = entity.speed or 1.6
    end,
    onUpdate = function(entity, _dt, elapsed)
        local offset = math.sin(elapsed * entity.speed) * entity.amplitude
        entity.position.x = entity.originX + offset
    end
})

-- Cristal / Moeda colecionável com rotação contínua e levitação suave
AstronBehaviors.register("CollectibleOrb", {
    onStart = function(entity)
        entity.baseY = entity.position.y
    end,
    onUpdate = function(entity, dt, elapsed)
        entity.rotation.y = (entity.rotation.y + 120.0 * dt) % 360.0
        entity.position.y = entity.baseY + math.sin(elapsed * 3.2) * 0.25
    end
})

-- Farol / Poste de iluminação urbana para mapas de Roleplay (Projeto RP)
AstronBehaviors.register("StreetLampCycle", {
    onUpdate = function(entity, _dt, elapsed)
        local dayFactor = 0.5 + 0.5 * math.sin(elapsed * 0.15)
        entity.emissive = (dayFactor < 0.45) and 1.0 or 0.15
    end
})

function AstronBehaviors.tickAll(entities, dt, elapsed)
    for i = 1, #entities do
        local ent = entities[i]
        local behavior = AstronBehaviors.registered[ent.behavior]
        if behavior and behavior.onUpdate then
            behavior.onUpdate(ent, dt, elapsed)
        end
    end
end

return AstronBehaviors
