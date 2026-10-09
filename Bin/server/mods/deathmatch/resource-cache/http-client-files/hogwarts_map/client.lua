-- ====================================================================
-- HOGWARTS CASTLE - GMOD MAP ENTEGRASYONU (TMOD ASSET MANAGER)
-- ====================================================================

-- Haritanın yerleştirileceği tam hedef koordinat
local HOGWARTS_POS = {
    x = -2407.20044,
    y = -2524.20020,
    z = 41.55014
}

local mapPropId = nil
local beaconMarker = nil
local beaconBlip = nil

addEventHandler("onClientResourceStart", resourceRoot, function()
    outputChatBox("==================================================", 0, 200, 255)
    outputChatBox("🏰 [TMOD] Hogwarts Haritası (OBJ + Bullet Collision) Yükleniyor...", 255, 255, 0)

    -- TMOD Asset Manager üzerinden OBJ modelini belleğe al
    local modelHandle = tmodLoadModel("targut-hogwartsss.obj")

    if modelHandle and modelHandle ~= "" then
        -- Modeli statik (kütle 0) olarak dünyamıza ve Bullet Physics simülasyonuna ekle
        mapPropId = tmodSpawnProp(modelHandle, HOGWARTS_POS.x, HOGWARTS_POS.y, HOGWARTS_POS.z, 0, 0, 0, 0, true)
        outputChatBox("🏰 [TMOD] Hogwarts Haritası başarıyla yerleştirildi! (Prop ID: " .. tostring(mapPropId) .. ")", 0, 255, 0)
        outputChatBox("👉 Haritaya ışınlanmak için '/hogwarts' yazın.", 0, 255, 255)

        -- Radar üzerinde Hogwarts simgesi (Harita ve Radar Blip'i)
        beaconBlip = createBlip(HOGWARTS_POS.x, HOGWARTS_POS.y, HOGWARTS_POS.z, 32, 2, 255, 0, 0, 255, 0, 99999.0)
        -- Giriş noktasında görsel ışık sütunu / marker
        beaconMarker = createMarker(HOGWARTS_POS.x, HOGWARTS_POS.y, HOGWARTS_POS.z, "cylinder", 4.0, 0, 200, 255, 150)
    else
        outputChatBox("❌ [TMOD] HATA: Hogwarts OBJ modeli yüklenemedi!", 255, 50, 50)
    end
    outputChatBox("==================================================", 0, 200, 255)
end)

-- Işınlanma Komutu
addCommandHandler("hogwarts", function()
    -- Kalenin avlu/büyük salon zeminine iniş
    setElementPosition(localPlayer, HOGWARTS_POS.x, HOGWARTS_POS.y, HOGWARTS_POS.z + 4.0)
    outputChatBox("✨ Hogwarts Kalesi'ne ışınlandınız! (Koordinat: " .. string.format("%.1f, %.1f, %.1f", HOGWARTS_POS.x, HOGWARTS_POS.y, HOGWARTS_POS.z + 4.0) .. ")", 0, 255, 100)
end)
