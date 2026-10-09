-- ====================================================================
-- FBX CITY MAP - ENTEGRASYONU (TMOD ASSET MANAGER & BULLET COLLISION)
-- ====================================================================

-- Haritanin yerlestirilecegi hedef dunya koordinati (havada asili sabit yukseklik)
local MAP_ORIGIN = {
    x = -315.19327,
    y = -1203.99744,
    z = 275.00000
}

-- Kati asfalt cadde meydani isinlanma koordinati (Zemin: 281.80m, Inis: 283.50m)
local CITY_SPAWN = {
    x = -315.19327,
    y = -1203.99744,
    z = 283.50
}

local mapPropId = nil
local beaconMarker = nil
local beaconBlip = nil

addEventHandler("onClientResourceStart", resourceRoot, function()
    outputChatBox("==================================================", 0, 200, 255)
    outputChatBox("[TMOD] FBX Sehir Haritasi (FBX Dokulari + Bullet Collision) Yukleniyor...", 255, 255, 0)

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
        outputChatBox("[TMOD] FBX Sehir Haritasi basariyla yerlestirildi! (Prop ID: " .. tostring(mapPropId) .. ")", 0, 255, 0)
        outputChatBox("Sehir Haritasina isinlanmak icin '/city' veya '/mapfbx' yazin.", 0, 255, 255)

        -- Radar uzerinde Sehir Haritasi simgesi
        beaconBlip = createBlip(CITY_SPAWN.x, CITY_SPAWN.y, CITY_SPAWN.z, 38, 2, 0, 200, 255, 255, 0, 99999.0)
        -- Giris noktasinda gorsel isik sutunu / marker
        beaconMarker = createMarker(CITY_SPAWN.x, CITY_SPAWN.y, CITY_SPAWN.z - 1.0, "cylinder", 4.0, 0, 255, 200, 150)
    else
        outputChatBox("[TMOD] HATA: FBX Sehir Haritasi modeli yuklenemedi!", 255, 50, 50)
        outputChatBox("[TMOD] Debug: city_map.fbx dosyasi bulunamadi.", 255, 100, 100)
    end
    outputChatBox("==================================================", 0, 200, 255)
end)

-- Isinlanma Komutlari
local function teleportToCity()
    -- Haritanin kati asfalt meydanina yumusak inis
    setElementPosition(localPlayer, CITY_SPAWN.x, CITY_SPAWN.y, CITY_SPAWN.z)
    outputChatBox("FBX Sehir Haritasi'na isinlandiniz! (Koordinat: " .. string.format("%.1f, %.1f, %.1f", CITY_SPAWN.x, CITY_SPAWN.y, CITY_SPAWN.z) .. ")", 0, 255, 100)
    outputChatBox("Haritada binalar, yollar ve catilar Bullet Physics zemin carpismasiyla tam etkindir.", 200, 255, 200)
end

addCommandHandler("city", teleportToCity)
addCommandHandler("mapfbx", teleportToCity)
