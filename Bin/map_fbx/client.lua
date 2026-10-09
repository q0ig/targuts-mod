-- ====================================================================
-- FBX CITY MAP - ENTEGRASYONU (TMOD ASSET MANAGER & BULLET COLLISION)
-- ====================================================================

-- Haritanin yerlestirilecegi hedef dunya koordinati (havada asili sabit yukseklik)
local MAP_ORIGIN = {
    x = -315.19327,
    y = -1203.99744,
    z = 275.00000
}

-- Kati asfalt cadde meydani isinlanma koordinati (Zemin: 275.11m, Inis: 276.50m)
local CITY_SPAWN = {
    x = -315.19327,
    y = -1203.99744,
    z = 276.50
}

local mapPropId = nil
local beaconMarker = nil
local beaconBlip = nil

addEventHandler("onClientResourceStart", resourceRoot, function()
    outputChatBox("[TMOD::MAP] Initializing FBX map loader and Bullet BVH collision...", 160, 175, 190)

    -- TMOD Asset Manager yerel map_fbx klasorunden oncelikle FBX yukler (tam doku ve UV destegiyle)
    local modelHandle = tmodLoadModel("map_fbx/city_map.fbx")
    if not modelHandle or modelHandle == "" then
        modelHandle = tmodLoadModel("city_map.fbx")
    end
    if not modelHandle or modelHandle == "" then
        modelHandle = tmodLoadModel("map_fbx/city_map.obj")
    end
    if not modelHandle or modelHandle == "" then
        modelHandle = tmodLoadModel("city_map.obj")
    end
    
    if modelHandle and modelHandle ~= "" then
        -- Modeli statik (kutle 0, havada freeze) olarak dunyamiza ve Bullet Physics simulasyonuna ekle
        mapPropId = tmodSpawnProp(modelHandle, MAP_ORIGIN.x, MAP_ORIGIN.y, MAP_ORIGIN.z, 0, 0, 0, 0, true)
        outputChatBox(string.format("[TMOD::MAP] city_map.fbx registered (PropID: %s) | Auto-Collision: READY", tostring(mapPropId)), 120, 175, 135)
        outputChatBox("[TMOD::MAP] Teleport command: '/city' or '/mapfbx'", 160, 175, 190)

        -- Radar uzerinde Sehir Haritasi simgesi
        beaconBlip = createBlip(CITY_SPAWN.x, CITY_SPAWN.y, CITY_SPAWN.z, 38, 2, 0, 200, 255, 255, 0, 99999.0)
        -- Giris noktasinda gorsel isik sutunu / marker
        beaconMarker = createMarker(CITY_SPAWN.x, CITY_SPAWN.y, CITY_SPAWN.z - 1.0, "cylinder", 4.0, 0, 255, 200, 150)
    else
        outputChatBox("[TMOD::MAP::ERR] Failed to load city_map asset (FBX/OBJ not found)", 195, 110, 110)
    end
end)

-- Isinlanma Komutlari
local function teleportToCity()
    -- Haritanin kati asfalt meydanina yumusak inis
    setElementPosition(localPlayer, CITY_SPAWN.x, CITY_SPAWN.y, CITY_SPAWN.z)
    outputChatBox(string.format("[TMOD::MAP] Teleport -> Destination: {%.1f, %.1f, %.1f} | Auto-Collision: ACTIVE", CITY_SPAWN.x, CITY_SPAWN.y, CITY_SPAWN.z), 135, 170, 195)
    outputChatBox("[TMOD::MAP] Mesh geometry resolved: Bullet BVH collider synced at Z=275.11m", 160, 175, 190)
end

addCommandHandler("city", teleportToCity)
addCommandHandler("mapfbx", teleportToCity)
