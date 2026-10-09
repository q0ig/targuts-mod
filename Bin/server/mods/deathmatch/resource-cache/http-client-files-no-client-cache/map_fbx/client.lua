-- ====================================================================
-- FBX CITY MAP - ENTEGRASYONU (TMOD ASSET MANAGER & BULLET COLLISION)
-- ====================================================================

-- Haritanin yerlestirilecegi hedef koordinat
local CITY_POS = {
    x = -2407.20044,
    y = -2524.20020,
    z = 81.55014  -- 41.55014 + 40.0
}

local mapPropId = nil
local beaconMarker = nil
local beaconBlip = nil

addEventHandler("onClientResourceStart", resourceRoot, function()
    outputChatBox("==================================================", 0, 200, 255)
    outputChatBox("[TMOD] FBX Sehir Haritasi (OBJ + Bullet Collision) Yukleniyor...", 255, 255, 0)

    -- MTA resource file system uzerinden dosyanin gercek yolunu bul.
    -- Client tarafinda resource dosyalari indirme cache'ine yerlesir,
    -- dogrudan "city_map.obj" olarak erisim mumkun degildir.
    -- fileExists + fileOpen ile dosyanin indirildigi gercek yolu cozumluyoruz.
    local modelHandle = nil
    
    -- Oncelikle MTA'nin resource dosya yolu ile dene
    local resRoot = ":" .. getResourceName(getThisResource()) .. "/"
    local actualPath = resRoot .. "city_map.obj"
    
    -- tmodLoadModel fonksiyonu istemci dosya sisteminde arar,
    -- bu yuzden MTA cache yolunu elle cikar
    if fileExists("city_map.obj") then
        -- Dosya resource cache'inde var, gecici olarak kopyala
        local fIn = fileOpen("city_map.obj", true)
        if fIn then
            local data = fileRead(fIn, fileGetSize(fIn))
            fileClose(fIn)
            
            -- MTA'nin mods/deathmatch/resources/ altina kaydet
            -- boylece CTModAssetManager dosyayi bulabilir
            local outPath = "map_fbx_cache/city_map.obj"
            if not fileExists(outPath) then
                local fOut = fileCreate(outPath)
                if fOut then
                    fileWrite(fOut, data)
                    fileClose(fOut)
                end
            end
        end
    end
    
    -- TMOD Asset Manager uzerinden modeli MTA yerel kaynak yoluyla yukle
    modelHandle = tmodLoadModel(actualPath)
    if not modelHandle or modelHandle == "" then
        modelHandle = tmodLoadModel("map_fbx/city_map.obj")
    end
    if not modelHandle or modelHandle == "" then
        modelHandle = tmodLoadModel("city_map.obj")
    end
    if not modelHandle or modelHandle == "" then
        modelHandle = tmodLoadModel("map_fbx_cache/city_map.obj")
    end
    
    if modelHandle and modelHandle ~= "" then
        -- Modeli statik (kutle 0) olarak dunyamiza ve Bullet Physics simulasyonuna ekle
        mapPropId = tmodSpawnProp(modelHandle, CITY_POS.x, CITY_POS.y, CITY_POS.z, 0, 0, 0, 0, true)
        outputChatBox("[TMOD] FBX Sehir Haritasi basariyla yerlestirildi! (Prop ID: " .. tostring(mapPropId) .. ")", 0, 255, 0)
        outputChatBox("Sehir Haritasina isinlanmak icin '/city' veya '/mapfbx' yazin.", 0, 255, 255)

        -- Radar uzerinde Sehir Haritasi simgesi
        beaconBlip = createBlip(CITY_POS.x, CITY_POS.y, CITY_POS.z, 38, 2, 0, 200, 255, 255, 0, 99999.0)
        -- Giris noktasinda gorsel isik sutunu / marker
        beaconMarker = createMarker(CITY_POS.x, CITY_POS.y, CITY_POS.z, "cylinder", 4.0, 0, 255, 200, 150)
    else
        outputChatBox("[TMOD] HATA: FBX Sehir Haritasi modeli yuklenemedi!", 255, 50, 50)
        outputChatBox("[TMOD] Debug: city_map.obj dosyasi bulunamadi.", 255, 100, 100)
    end
    outputChatBox("==================================================", 0, 200, 255)
end)

-- Isinlanma Komutlari
local function teleportToCity()
    -- Haritanin sokak seviyesine inis (+2m guvenlik payi)
    setElementPosition(localPlayer, CITY_POS.x, CITY_POS.y, CITY_POS.z + 2.0)
    outputChatBox("FBX Sehir Haritasi'na isinlandiniz! (Koordinat: " .. string.format("%.1f, %.1f, %.1f", CITY_POS.x, CITY_POS.y, CITY_POS.z + 2.0) .. ")", 0, 255, 100)
    outputChatBox("Haritada binalar, yollar ve catilar Bullet Physics zemin carpismasiyla tam etkindir.", 200, 255, 200)
end

addCommandHandler("city", teleportToCity)
addCommandHandler("mapfbx", teleportToCity)
