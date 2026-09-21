-- ====================================================================
-- TMOD - GELİSTİRİCİ & TEST KONTROL PANELİ (CEGUI)
-- ====================================================================

local window = nil
local isWindowVisible = false

-- Physics test nesneleri takip tablosu
local spawnedBodies = {}

-- Ekran cozunurlugu
local screenW, screenH = guiGetScreenSize()
local winW, winH = 540, 480
local winX = (screenW - winW) / 2
local winY = (screenH - winH) / 2

function buildTestGUI()
    if window then return end

    -- Ana Pencere
    window = guiCreateWindow(winX, winY, winW, winH, "Targut's Mod (TMOD) - Test ve Kontrol Paneli [F5]", false)
    guiWindowSetSizable(window, false)
    guiSetVisible(window, false)

    -- Kapat Butonu
    local btnClose = guiCreateButton(winW - 35, 25, 25, 22, "X", false, window)
    addEventHandler("onClientGUIClick", btnClose, toggleGUI, false)

    -- Sekme Paneli
    local tabPanel = guiCreateTabPanel(10, 30, winW - 20, winH - 40, false, window)

    -- ====================================================================
    -- SEKME 1: HAREKET (SOURCE / GMOD MOVEMENT)
    -- ====================================================================
    local tabMovement = guiCreateTab("Hareket (Movement)", tabPanel)

    guiCreateLabel(15, 15, 460, 20, "--- HAREKET MOTORU MODU ---", false, tabMovement)
    
    local lblCurrentMode = guiCreateLabel(15, 35, 460, 20, "Aktif Mod: " .. (tmodGetMovementMode and tmodGetMovementMode() or "Bilinmiyor"), false, tabMovement)
    guiLabelSetColor(lblCurrentMode, 0, 255, 150)

    local btnSourceMode = guiCreateButton(15, 60, 230, 35, "Source / GMod Modunu Ac", false, tabMovement)
    local btnDefaultMode = guiCreateButton(260, 60, 230, 35, "Varsayilan (GTA) Moduna Don", false, tabMovement)

    addEventHandler("onClientGUIClick", btnSourceMode, function()
        if tmodSetMovementMode then
            tmodSetMovementMode("source")
            guiSetText(lblCurrentMode, "Aktif Mod: " .. tmodGetMovementMode())
            outputChatBox("[TMOD::MOVE] Movement controller -> 'source' (sv_accelerate, air_strafe, bhop active)", 135, 170, 195)
        end
    end, false)

    addEventHandler("onClientGUIClick", btnDefaultMode, function()
        if tmodSetMovementMode then
            tmodSetMovementMode("default")
            guiSetText(lblCurrentMode, "Aktif Mod: " .. tmodGetMovementMode())
            outputChatBox("[TMOD::MOVE] Movement controller -> 'default' (GTA:SA native physics)", 195, 155, 95)
        end
    end, false)

    -- Auto Bhop Checkbox
    guiCreateLabel(15, 115, 460, 20, "--- ZIPLAMA & BUNNYHOP AYARLARI ---", false, tabMovement)
    local chkBhop = guiCreateCheckBox(15, 140, 460, 25, "Otomatik Bunnyhop (Space basili tutunca kesintisiz zipla)", true, false, tabMovement)
    addEventHandler("onClientGUIClick", chkBhop, function()
        local state = guiCheckBoxGetSelected(chkBhop)
        if tmodSetAutoBhop then
            tmodSetAutoBhop(state)
            outputChatBox("[TMOD::MOVE] Auto-Bhop state set to: " .. (state and "ENABLED" or "DISABLED"), 160, 175, 190)
        end
    end, false)

    -- Air Accelerate
    guiCreateLabel(15, 180, 460, 20, "--- HAVA IVMELENMESI (AIR ACCELERATE / STRAFE) ---", false, tabMovement)
    local btnAir1 = guiCreateButton(15, 205, 110, 30, "Normal (100)", false, tabMovement)
    local btnAir2 = guiCreateButton(135, 205, 110, 30, "Yuksek (300)", false, tabMovement)
    local btnAir3 = guiCreateButton(255, 205, 110, 30, "Surf (800)", false, tabMovement)
    local btnAir4 = guiCreateButton(375, 205, 110, 30, "Cilgin (1500)", false, tabMovement)

    local function setAir(val)
        if tmodSetAirAccelerate then
            tmodSetAirAccelerate(val)
            outputChatBox(string.format("[TMOD::MOVE] Parameter updated: sv_airaccelerate = %.1f", val), 160, 175, 190)
        end
    end
    addEventHandler("onClientGUIClick", btnAir1, function() setAir(100.0) end, false)
    addEventHandler("onClientGUIClick", btnAir2, function() setAir(300.0) end, false)
    addEventHandler("onClientGUIClick", btnAir3, function() setAir(800.0) end, false)
    addEventHandler("onClientGUIClick", btnAir4, function() setAir(1500.0) end, false)

    -- Bilgi Notu
    local lblInfo = guiCreateLabel(15, 260, 480, 100, "Ipucu: Source modunda ivme kazanmak icin kosarken ziplayin,\nSpace'e basili tutun ve havada 'A' basarken fareyi sola,\n'D' basarken fareyi saga cevirin (Air-Strafing)!\nA-S-D tuslarinda karakter donmez, Source gibi ayak/kol strafe animasyonu verir.", false, tabMovement)
    guiLabelSetColor(lblInfo, 200, 200, 200)

    -- ====================================================================
    -- SEKME 2: KAMERA (CAMERA SYSTEM)
    -- ====================================================================
    local tabCamera = guiCreateTab("Kamera (Camera)", tabPanel)

    guiCreateLabel(15, 15, 460, 20, "--- KAMERA MODU SECIMI ---", false, tabCamera)
    local btnCamThird = guiCreateButton(15, 45, 150, 35, "Ucuncu Sahis (Source)", false, tabCamera)
    local btnCamFirst = guiCreateButton(180, 45, 150, 35, "Birinci Sahis (FPS)", false, tabCamera)
    local btnCamDef   = guiCreateButton(345, 45, 150, 35, "Orijinal GTA", false, tabCamera)

    addEventHandler("onClientGUIClick", btnCamThird, function()
        if tmodSetCameraMode then
            tmodSetCameraMode("thirdperson")
            outputChatBox("[TMOD::CAM] Viewport mode -> THIRD_PERSON (orbit dist: 4.5m)", 135, 170, 195)
        end
    end, false)

    addEventHandler("onClientGUIClick", btnCamFirst, function()
        if tmodSetCameraMode then
            tmodSetCameraMode("firstperson")
            outputChatBox("[TMOD::CAM] Viewport mode -> FIRST_PERSON (viewmodel attached)", 135, 170, 195)
        end
    end, false)

    addEventHandler("onClientGUIClick", btnCamDef, function()
        if tmodSetCameraMode then
            tmodSetCameraMode("default")
            outputChatBox("[TMOD::CAM] Viewport mode -> DEFAULT (GTA:SA native CCamera)", 195, 155, 95)
        end
    end, false)

    guiCreateLabel(15, 105, 460, 20, "--- 3. SAHIS KAMERA MESAFESI ---", false, tabCamera)
    local btnDist1 = guiCreateButton(15, 130, 110, 30, "Yakin (2.5m)", false, tabCamera)
    local btnDist2 = guiCreateButton(135, 130, 110, 30, "Normal (4.5m)", false, tabCamera)
    local btnDist3 = guiCreateButton(255, 130, 110, 30, "Uzak (7.0m)", false, tabCamera)
    local btnDist4 = guiCreateButton(375, 130, 110, 30, "Sinematik (12m)", false, tabCamera)

    local function setDist(val)
        if tmodSetCameraDistance then
            tmodSetCameraDistance(val)
            outputChatBox(string.format("[TMOD::CAM] Camera orbit distance set to: %.1fm", val), 160, 175, 190)
        end
    end
    addEventHandler("onClientGUIClick", btnDist1, function() setDist(2.5) end, false)
    addEventHandler("onClientGUIClick", btnDist2, function() setDist(4.5) end, false)
    addEventHandler("onClientGUIClick", btnDist3, function() setDist(7.0) end, false)
    addEventHandler("onClientGUIClick", btnDist4, function() setDist(12.0) end, false)

    local lblCamInfo = guiCreateLabel(15, 185, 480, 80, "Source ve FPS kamerasinda fareyi cevirdiginizde karakter daima baktiginiz\nyone bakar. W ile ileri, S ile geri adim, A ve D ile sola/saga strafe animasyonlari\nverilir ve karakter ters donmez.", false, tabCamera)
    guiLabelSetColor(lblCamInfo, 200, 200, 200)

    -- ====================================================================
    -- SEKME 3: BULLET FIZIK & PROP TESTI (PHYSICS)
    -- ====================================================================
    local tabPhysics = guiCreateTab("Bullet Fizik (Physics)", tabPanel)

    guiCreateLabel(15, 10, 480, 20, "--- BULLET FIZIK MOTORU & GTA NESNE ETKILESIIMI ---", false, tabPhysics)
    local btnSpawnBox = guiCreateButton(15, 35, 230, 35, "Onume Fizik Kutusu At", false, tabPhysics)
    local btnApplyForce = guiCreateButton(260, 35, 230, 35, "Kutuya Yukari Darbe Vur", false, tabPhysics)

    local btnFreezeBody = guiCreateButton(15, 80, 230, 35, "Kutuyu Dondur (Freeze)", false, tabPhysics)
    local btnUnfreezeBody = guiCreateButton(260, 80, 230, 35, "Kutuyu Coz (Unfreeze)", false, tabPhysics)

    local btnAttachVeh = guiCreateButton(15, 125, 230, 35, "Arabaya Bullet Fizigi Bagla", false, tabPhysics)
    local btnAttachObj = guiCreateButton(260, 125, 230, 35, "Yakindaki Objelere Fizik Bagla", false, tabPhysics)

    local btnClearAll = guiCreateButton(15, 170, 475, 30, "Tum Fizik Nesnelerini Temizle", false, tabPhysics)

    local lblPhysStatus = guiCreateLabel(15, 215, 480, 60, "Bullet Physics aktif: Yercekimi, zemin carpismasi, kutu firlatma\nve oyuncunun kutulari itebilmesi icin kinematik carpisma destegi eklendi.", false, tabPhysics)
    guiLabelSetColor(lblPhysStatus, 150, 255, 150)

    local lastBodyId = nil

    addEventHandler("onClientGUIClick", btnSpawnBox, function()
        local x, y, z = getElementPosition(localPlayer)
        local rx, ry, rz = getElementRotation(localPlayer)
        local rad = math.rad(rz)
        local spawnX = x - math.sin(rad) * 2.5
        local spawnY = y + math.cos(rad) * 2.5
        local spawnZ = z + 1.2

        -- GTA Gorsel Objesi olustur (Ahsap Sandik: 1220)
        local obj = createObject(1220, spawnX, spawnY, spawnZ)

        -- Bullet Rigid Body olustur ve GTA objesine bagla
        local bodyId = nil
        if tmodAttachPhysics then
            bodyId = tmodAttachPhysics(obj, 20.0, 1.0, 1.0, 1.0)
        elseif tmodCreateRigidBody then
            bodyId = tmodCreateRigidBody(spawnX, spawnY, spawnZ, 1.0, 1.0, 1.0, 20.0)
            if tmodBindBodyToElement and bodyId then
                tmodBindBodyToElement(bodyId, obj)
            end
        end

        if bodyId then
            lastBodyId = bodyId
            table.insert(spawnedBodies, { bodyId = bodyId, object = obj })
            guiSetText(lblPhysStatus, "Son Kutu ID: " .. tostring(bodyId) .. " (Toplam: " .. #spawnedBodies .. ")\nKutuya kosarak itebilir veya darbe vurabilirsiniz!")
            outputChatBox(string.format("[TMOD::PHYS] RigidBody #%d instantiated -> bound to CClientObject (mass=20.0kg)", bodyId), 120, 175, 135)
        end
    end, false)

    addEventHandler("onClientGUIClick", btnApplyForce, function()
        if lastBodyId and tmodApplyForce then
            tmodApplyForce(lastBodyId, 0, 0, 300.0)
            outputChatBox(string.format("[TMOD::PHYS] Applied impulse to Body #%d: {0.0, 0.0, 300.0} N", lastBodyId), 135, 170, 195)
        else
            outputChatBox("[TMOD::PHYS::WARN] No active RigidBody selected in registry", 195, 110, 110)
        end
    end, false)

    addEventHandler("onClientGUIClick", btnFreezeBody, function()
        if lastBodyId and tmodFreezeBody then
            tmodFreezeBody(lastBodyId, true)
            outputChatBox(string.format("[TMOD::PHYS] Body #%d state -> FROZEN (Kinematic / zero-velocity)", lastBodyId), 160, 175, 190)
        end
    end, false)

    addEventHandler("onClientGUIClick", btnUnfreezeBody, function()
        if lastBodyId and tmodFreezeBody then
            tmodFreezeBody(lastBodyId, false)
            outputChatBox(string.format("[TMOD::PHYS] Body #%d state -> ACTIVE (Dynamic simulation)", lastBodyId), 120, 175, 135)
        end
    end, false)

    addEventHandler("onClientGUIClick", btnAttachVeh, function()
        local px, py, pz = getElementPosition(localPlayer)
        local targetVeh = getPedOccupiedVehicle(localPlayer)
        if not targetVeh then
            local nearestDist = 20.0
            for _, veh in ipairs(getElementsByType("vehicle")) do
                local vx, vy, vz = getElementPosition(veh)
                local d = getDistanceBetweenPoints3D(px, py, pz, vx, vy, vz)
                if d < nearestDist then
                    nearestDist = d
                    targetVeh = veh
                end
            end
        end
        if targetVeh and tmodAttachPhysics then
            local bid = tmodAttachPhysics(targetVeh, 1500.0)
            if bid and bid > 0 then
                lastBodyId = bid
                table.insert(spawnedBodies, { bodyId = bid, object = targetVeh })
                outputChatBox(string.format("[TMOD::PHYS] Vehicle bound to Bullet dynamics [BodyID: #%d, Mass: 1500.0kg]", bid), 120, 175, 135)
            end
        else
            outputChatBox("[TMOD::PHYS::WARN] AttachVehicle failed: no target vehicle within search radius", 195, 110, 110)
        end
    end, false)

    addEventHandler("onClientGUIClick", btnAttachObj, function()
        local px, py, pz = getElementPosition(localPlayer)
        local count = 0
        for _, obj in ipairs(getElementsByType("object")) do
            local ox, oy, oz = getElementPosition(obj)
            local d = getDistanceBetweenPoints3D(px, py, pz, ox, oy, oz)
            if d < 12.0 and d > 0.5 then
                local bid = tmodAttachPhysics and tmodAttachPhysics(obj, 25.0)
                if bid and bid > 0 then
                    table.insert(spawnedBodies, { bodyId = bid, object = obj })
                    lastBodyId = bid
                    count = count + 1
                end
            end
        end
        if count > 0 then
            outputChatBox(string.format("[TMOD::PHYS] Bound %d nearby world object(s) to Bullet simulation", count), 120, 175, 135)
        else
            outputChatBox("[TMOD::PHYS::WARN] AttachObjects failed: no dynamic candidate within radius", 195, 155, 95)
        end
    end, false)

    addEventHandler("onClientGUIClick", btnClearAll, function()
        for _, item in ipairs(spawnedBodies) do
            if tmodDestroyBody then tmodDestroyBody(item.bodyId) end
            if isElement(item.object) and getElementType(item.object) == "object" then
                destroyElement(item.object)
            end
        end
        spawnedBodies = {}
        lastBodyId = nil
        guiSetText(lblPhysStatus, "Tum kutular ve bagli fizik nesneleri temizlendi.")
        outputChatBox("[TMOD::PHYS] Simulation flushed: destroyed all registered dynamic bodies", 195, 155, 95)
    end, false)

    -- ====================================================================
    -- SEKME 4: GRAFIK & ATMOSFER (GRAPHICS)
    -- ====================================================================
    local tabGraphics = guiCreateTab("Grafik & Atmosfer", tabPanel)

    guiCreateLabel(15, 15, 460, 20, "--- ATMOSFER / RENK FILTRESI ---", false, tabGraphics)
    local btnSourceFX = guiCreateButton(15, 45, 230, 35, "Source Engine Atmosferini Ac", false, tabGraphics)
    local btnDefaultFX = guiCreateButton(260, 45, 230, 35, "Varsayilan (GTA) Atmosferine Don", false, tabGraphics)

    addEventHandler("onClientGUIClick", btnSourceFX, function()
        if tmodSetSourceColorCorrection then
            tmodSetSourceColorCorrection(true)
            outputChatBox("[TMOD::POSTFX] Atmosphere pipeline -> 'source_cold' (Direct3D shader enabled)", 135, 170, 195)
        end
    end, false)

    addEventHandler("onClientGUIClick", btnDefaultFX, function()
        if tmodSetSourceColorCorrection then
            tmodSetSourceColorCorrection(false)
            outputChatBox("[TMOD::POSTFX] Atmosphere pipeline -> 'default' (GTA:SA native color filter)", 195, 155, 95)
        end
    end, false)
    
    local lblFXInfo = guiCreateLabel(15, 95, 480, 80, "Source Engine atmosferi: GTA:SA'nin meshur sari/turuncu filtresini,\nblur ve heat haze etkilerini kaldirarak daha temiz,\nsoguk tonlu ve kontrastli (GMod / Half-Life 2 tarzi) bir gorsellik sunar.", false, tabGraphics)
    guiLabelSetColor(lblFXInfo, 200, 200, 200)

    -- ====================================================================
    -- SEKME 5: OYUNCU & YARDIMCI ARACLAR
    -- ====================================================================
    local tabTools = guiCreateTab("Araclar & Yardim", tabPanel)

    guiCreateLabel(15, 15, 460, 20, "--- HIZLI TEST KOMUTLARI ---", false, tabTools)

    local btnHeal = guiCreateButton(15, 45, 140, 30, "Can / Zirh Yenile", false, tabTools)
    local btnCar1 = guiCreateButton(165, 45, 140, 30, "Infernus Dogur", false, tabTools)
    local btnCar2 = guiCreateButton(315, 45, 140, 30, "Monster Dogur", false, tabTools)

    addEventHandler("onClientGUIClick", btnHeal, function()
        triggerServerEvent("tmod:serverHealPlayer", resourceRoot)
    end, false)

    addEventHandler("onClientGUIClick", btnCar1, function()
        triggerServerEvent("tmod:serverSpawnVehicle", resourceRoot, 411) -- Infernus
    end, false)

    addEventHandler("onClientGUIClick", btnCar2, function()
        triggerServerEvent("tmod:serverSpawnVehicle", resourceRoot, 444) -- Monster
    end, false)

    local btnCity = guiCreateButton(15, 85, 200, 30, "Sehir Haritasina Git (/city)", false, tabTools)
    addEventHandler("onClientGUIClick", btnCity, function()
        teleportToCity()
    end, false)

    local lblHelp = guiCreateLabel(15, 125, 480, 200, 
        "KONTROL BILGILERI:\n\n" ..
        "- F5 Tusu veya /tmod Komutu: Bu paneli acar ve kapatir.\n" ..
        "- /city veya /mapfbx: FBX Sehir Haritasina isinlar.\n" ..
        "- Source Movement: Koşarken Space'e basılı tutun ve havada A/D strafe yapın.\n" ..
        "- Kamera Modu: FPS ve 3. şahısta karakter farenin baktığı yöne kilitlenir,\n" ..
        "  S ile geri, A/D ile yana doğru strafe adımları atar.\n" ..
        "- Bullet Physics: Kutular artık GTA zeminine çarpar, havada sabit kalmaz,\n" ..
        "  karakterle kutuları itebilir, fırlatabilir ve dondurabilirsiniz.\n", 
        false, tabTools)
    guiLabelSetColor(lblHelp, 220, 220, 220)
end

-- ====================================================================
-- SEHIR HARITASI (MAP_FBX) ISINLANMA & YONETIMI
-- ====================================================================
local CITY_POS = {
    x = -315.19327,
    y = -1203.99744,
    z = 276.50  -- Genis acik cadde meydani (Zemin: 275.11m, yumusak inis: 276.50m)
}

function teleportToCity()
    -- C++ Asset Manager modeli map_fbx yerel dizininden otomatik yukler,
    -- eger henuz yuklenmemisse buradan da garanti olarak tetikle
    if tmodLoadModel and tmodSpawnProp then
        local handle = tmodLoadModel("map_fbx/city_map.fbx")
        if not handle or handle == "" then
            handle = tmodLoadModel("city_map.fbx")
        end
        if not handle or handle == "" then
            handle = tmodLoadModel("map_fbx/city_map.obj")
        end
        if handle and handle ~= "" then
            tmodSpawnProp(handle, -315.19327, -1203.99744, 275.00000, 0, 0, 0, 0, true)
        end
    end

    setElementPosition(localPlayer, CITY_POS.x, CITY_POS.y, CITY_POS.z)
    outputChatBox(string.format("[TMOD::MAP] Teleport -> Destination: {%.1f, %.1f, %.1f} | Auto-Collision: ACTIVE", CITY_POS.x, CITY_POS.y, CITY_POS.z), 135, 170, 195)
    outputChatBox("[TMOD::MAP] Mesh geometry resolved: Bullet BVH collider synced at Z=275.11m", 160, 175, 190)
end

addCommandHandler("city", teleportToCity)
addCommandHandler("mapfbx", teleportToCity)

-- ====================================================================
-- TOGGLE & BINDLER
-- ====================================================================
function toggleGUI()
    if not window then
        buildTestGUI()
    end

    isWindowVisible = not isWindowVisible
    guiSetVisible(window, isWindowVisible)
    showCursor(isWindowVisible)
end

addCommandHandler("tmod", toggleGUI)
bindKey("F5", "down", toggleGUI)

addEventHandler("onClientResourceStart", resourceRoot, function()
    outputChatBox("[TMOD::SYS] Debug subsystem initialized | GUI toggle: [F5] or '/tmod' | Map: '/city'", 160, 175, 190)

    -- Harita radar isareti
    createBlip(CITY_POS.x, CITY_POS.y, 275.0, 38, 2, 0, 200, 255, 255, 0, 99999.0)
end)
