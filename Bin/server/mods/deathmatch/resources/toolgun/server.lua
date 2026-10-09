-- ====================================================================
-- GARRYS MOD TOOL GUN: IN-GAME MAP/PROP SPAWNER & EDITOR
-- Server-Side Synchronizer, Element Manager & Map Exporter
-- ====================================================================

local placedEntities = {}     -- [uniqueId] = { type, element, model/effectName, x, y, z, rx, ry, rz, extraData, creator }
local playerHistory = {}      -- [player] = { uniqueId1, uniqueId2, ... } (for Undo functionality)
local entityCounter = 0

-- ====================================================================
-- HELPER: Generate Unique Entity ID
-- ====================================================================
local function getNextEntityId()
    entityCounter = entityCounter + 1
    return entityCounter
end

-- ====================================================================
-- EVENT: Player Requests Item Placement
-- ====================================================================
addEvent("toolgun:serverPlaceItem", true)
addEventHandler("toolgun:serverPlaceItem", root, function(itemType, idOrName, x, y, z, rx, ry, rz, extraData)
    if not client then return end

    local uniqueId = getNextEntityId()
    local creatorName = getPlayerName(client)
    local entry = {
        id = uniqueId,
        type = itemType,
        x = tonumber(string.format("%.4f", x)),
        y = tonumber(string.format("%.4f", y)),
        z = tonumber(string.format("%.4f", z)),
        rx = tonumber(string.format("%.2f", rx or 0)),
        ry = tonumber(string.format("%.2f", ry or 0)),
        rz = tonumber(string.format("%.2f", rz or 0)),
        creator = creatorName,
        extraData = extraData or {}
    }

    if itemType == "object" then
        local model = tonumber(idOrName)
        if not model then return end

        local obj = createObject(model, entry.x, entry.y, entry.z, entry.rx, entry.ry, entry.rz)
        if not isElement(obj) then
            outputChatBox("#FF3333[Tool Gun] #FFFFFFObje olusturulamadi (Gecersiz Model: " .. tostring(model) .. ")!", client, 255, 255, 255, true)
            return
        end

        setElementData(obj, "toolgun:id", uniqueId)
        setElementData(obj, "toolgun:creator", creatorName)
        setElementData(obj, "toolgun:model", model)
        setElementCollisionsEnabled(obj, true)

        entry.element = obj
        entry.model = model

    elseif itemType == "light" then
        entry.r = extraData.r or 255
        entry.g = extraData.g or 255
        entry.b = extraData.b or 255
        entry.radius = extraData.radius or 8.0

        -- Broadcast light creation to all connected clients
        triggerClientEvent(root, "toolgun:clientSyncLightAdd", root, uniqueId, entry)

    elseif itemType == "effect" then
        entry.effectName = tostring(idOrName)

        -- Broadcast visual effect creation to all connected clients
        triggerClientEvent(root, "toolgun:clientSyncEffectAdd", root, uniqueId, entry)
    end

    placedEntities[uniqueId] = entry

    if not playerHistory[client] then
        playerHistory[client] = {}
    end
    table.insert(playerHistory[client], uniqueId)

    outputChatBox("#00FF88[Tool Gun] #FFFFFF" .. tostring(entry.type):upper() .. " basariyla yerlestirildi! (#" .. uniqueId .. ")", client, 255, 255, 255, true)
end)

-- ====================================================================
-- EVENT: Player Requests Deletion of Item
-- ====================================================================
addEvent("toolgun:serverDeleteItem", true)
addEventHandler("toolgun:serverDeleteItem", root, function(targetElementOrId)
    if not client then return end

    local targetId = nil
    if isElement(targetElementOrId) then
        targetId = getElementData(targetElementOrId, "toolgun:id")
    else
        targetId = tonumber(targetElementOrId)
    end

    if not targetId or not placedEntities[targetId] then
        outputChatBox("#FF9900[Tool Gun] #FFFFFFSilinecek nesne bulunamadi veya Tool Gun ile olusturulmamis!", client, 255, 255, 255, true)
        return
    end

    local entry = placedEntities[targetId]
    if entry.type == "object" and isElement(entry.element) then
        destroyElement(entry.element)
    elseif entry.type == "light" then
        triggerClientEvent(root, "toolgun:clientSyncLightRemove", root, targetId)
    elseif entry.type == "effect" then
        triggerClientEvent(root, "toolgun:clientSyncEffectRemove", root, targetId)
    end

    placedEntities[targetId] = nil
    outputChatBox("#FF4444[Tool Gun] #FFFFFFNesne kaldirildi (#" .. targetId .. ")!", client, 255, 255, 255, true)
end)

-- ====================================================================
-- EVENT: Initial Sync for Lights & Effects upon Player Join
-- ====================================================================
addEvent("toolgun:requestInitialSync", true)
addEventHandler("toolgun:requestInitialSync", root, function()
    if not client then return end

    local lights = {}
    local effects = {}

    for id, ent in pairs(placedEntities) do
        if ent.type == "light" then
            lights[id] = ent
        elseif ent.type == "effect" then
            effects[id] = ent
        end
    end

    triggerClientEvent(client, "toolgun:clientReceiveInitialSync", client, lights, effects)
end)

-- ====================================================================
-- COMMAND: /toolgun or /givegun (Gives Silenced 9mm & Tool)
-- ====================================================================
local function giveToolGunCommand(player)
    if not isElement(player) then return end
    giveWeapon(player, TOOLGUN_CONFIG.weaponId, 9999, true)
    setWeaponAmmo(player, TOOLGUN_CONFIG.weaponId, 9999, 9999)
    outputChatBox("#00FF88[Tool Gun] #FFFFFFGarry's Mod Tool Gun (Silenced 9mm) cantaniza verildi!", player, 255, 255, 255, true)
    outputChatBox("#00CCFF[Tool Gun] #FFFFFFMenu icin '#00FF88B#FFFFFF' tusuna basin veya #00FF88/toolgun#FFFFFF yazin.", player, 255, 255, 255, true)
end
addCommandHandler("toolgun", giveToolGunCommand)
addCommandHandler("givegun", giveToolGunCommand)

-- ====================================================================
-- COMMAND: /undo (Undo Last Placed Item)
-- ====================================================================
addEvent("toolgun:serverUndo", true)
addEventHandler("toolgun:serverUndo", root, function()
    local player = client
    if not isElement(player) then return end
    local hist = playerHistory[player]
    if not hist or #hist == 0 then
        outputChatBox("#FF9900[Tool Gun] #FFFFFFGeri alinacak nesne bulunamadi!", player, 255, 255, 255, true)
        return
    end

    local lastId = table.remove(hist)
    while lastId and not placedEntities[lastId] and #hist > 0 do
        lastId = table.remove(hist)
    end

    if lastId and placedEntities[lastId] then
        local entry = placedEntities[lastId]
        if entry.type == "object" and isElement(entry.element) then
            destroyElement(entry.element)
        elseif entry.type == "light" then
            triggerClientEvent(root, "toolgun:clientSyncLightRemove", root, lastId)
        elseif entry.type == "effect" then
            triggerClientEvent(root, "toolgun:clientSyncEffectRemove", root, lastId)
        end
        placedEntities[lastId] = nil
        outputChatBox("#FFCC00[Tool Gun] #FFFFFFSon nesne geri alindi (#" .. lastId .. ")!", player, 255, 255, 255, true)
    else
        outputChatBox("#FF9900[Tool Gun] #FFFFFFGeri alinacak nesne bulunamadi!", player, 255, 255, 255, true)
    end
end)

-- ====================================================================
-- COMMAND: /clearmap (Clear all Tool Gun spawned entities)
-- ====================================================================
addCommandHandler("clearmap", function(player)
    local count = 0
    for id, entry in pairs(placedEntities) do
        if entry.type == "object" and isElement(entry.element) then
            destroyElement(entry.element)
        elseif entry.type == "light" then
            triggerClientEvent(root, "toolgun:clientSyncLightRemove", root, id)
        elseif entry.type == "effect" then
            triggerClientEvent(root, "toolgun:clientSyncEffectRemove", root, id)
        end
        count = count + 1
    end

    placedEntities = {}
    playerHistory = {}
    outputChatBox("#FF4444[Tool Gun] #FFFFFFHaritadaki tum Tool Gun nesneleri (" .. count .. " adet) silindi!", root, 255, 255, 255, true)
end)

-- ====================================================================
-- COMMAND: /exportmap [filename] (Generates MTA Editor .map XML file)
-- ====================================================================
addEvent("toolgun:serverExportMap", true)
addEventHandler("toolgun:serverExportMap", root, function(filename)
    local player = client

    local name = filename
    if not name or name:gsub("%s+", "") == "" then
        name = "toolgun_exported.map"
    end
    if not name:find("%.map$") then
        name = name .. ".map"
    end

    local xmlLines = {}
    table.insert(xmlLines, '<map edf:definitions="editor_main">')
    table.insert(xmlLines, '    <!-- Generated by Garry\'s Mod Tool Gun Spawner & Editor for MTA:SA -->')
    table.insert(xmlLines, '    <!-- Export Date: ' .. os.date("%Y-%m-%d %H:%M:%S") .. ' -->')

    local objCount = 0
    local lightCount = 0
    local fxCount = 0

    for id, ent in pairs(placedEntities) do
        if ent.type == "object" then
            objCount = objCount + 1
            local line = string.format(
                '    <object id="object (%d)" model="%d" posX="%.4f" posY="%.4f" posZ="%.4f" rotX="%.2f" rotY="%.2f" rotZ="%.2f" dimension="0" interior="0" collisions="true" alpha="255" />',
                objCount, ent.model, ent.x, ent.y, ent.z, ent.rx, ent.ry, ent.rz
            )
            table.insert(xmlLines, line)
        elseif ent.type == "light" then
            lightCount = lightCount + 1
            local line = string.format(
                '    <!-- Light (%d): Radius=%.1f RGB=[%d, %d, %d] Pos=[%.4f, %.4f, %.4f] -->',
                lightCount, ent.radius or 8.0, ent.r or 255, ent.g or 255, ent.b or 255, ent.x, ent.y, ent.z
            )
            table.insert(xmlLines, line)
        elseif ent.type == "effect" then
            fxCount = fxCount + 1
            local line = string.format(
                '    <!-- Effect (%d): Name="%s" Pos=[%.4f, %.4f, %.4f] Rot=[%.2f, %.2f, %.2f] -->',
                fxCount, ent.effectName or "fx", ent.x, ent.y, ent.z, ent.rx, ent.ry, ent.rz
            )
            table.insert(xmlLines, line)
        end
    end

    table.insert(xmlLines, '</map>')
    local fullXml = table.concat(xmlLines, "\n")

    -- Save to server resource folder (e.g. exports/name.map or name.map)
    local filePath = name
    if fileExists(filePath) then
        fileDelete(filePath)
    end

    local file = fileCreate(filePath)
    if file then
        fileWrite(file, fullXml)
        fileClose(file)

        local targetMsg = isElement(player) and player or root
        outputChatBox("#00FF88[Tool Gun Map Export] #FFFFFFHarita basariyla kaydedildi!", targetMsg, 255, 255, 255, true)
        outputChatBox("#00CCFF[Dosya]: #FFFFFF" .. filePath .. " #777777(Toplam: " .. objCount .. " Obje, " .. lightCount .. " Isik, " .. fxCount .. " Efekt)", targetMsg, 255, 255, 255, true)
        outputChatBox("#AAAAAA[Bilgi]: Bu dosyayi dogrudan MTA Map Editor icine aktarabilir veya baska map resource'larinda kullanabilirsiniz.", targetMsg, 255, 255, 255, true)
    else
        local targetMsg = isElement(player) and player or root
        outputChatBox("#FF3333[Tool Gun] #FFFFFFDosya yazma hatasi olustu: " .. filePath, targetMsg, 255, 255, 255, true)
    end
end)

-- Clean up player history on quit
addEventHandler("onPlayerQuit", root, function()
    playerHistory[source] = nil
end)
