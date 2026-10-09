-- ====================================================================
-- GARRYS MOD TOOL GUN: IN-GAME MAP/PROP SPAWNER & EDITOR
-- Client-Side Controller, CEGUI Spawner, 3D Gizmo & Object Preview
-- ====================================================================

local sw, sh = guiGetScreenSize()

-- Runtime State
local isGuiOpen = false
local currentToolMode = "object"      -- "object", "light", "effect", "delete"
local currentItemData = nil           -- Selected item from catalog

-- Active Ghost / Editing Entity
local ghostElement = nil              -- Client-side preview entity
local ghostType = nil                 -- "object", "light", "effect"
local ghostPos = { x = 0, y = 0, z = 0 }
local ghostRot = { x = 0, y = 0, z = 0 }
local ghostLight = nil                -- For light preview
local ghostEffect = nil               -- For effect preview
local isEditingGhost = false

-- Synchronized Client Lights & Effects
local activeClientLights = {}         -- [uniqueId] = lightElement
local activeClientEffects = {}        -- [uniqueId] = effectElement

-- GUI Elements
local guiWindow = nil
local guiSearchBox = nil
local guiCategoryCombo = nil
local guiGridList = nil
local guiBtnSpawn = nil
local guiBtnDeleteMode = nil
local guiBtnClose = nil
local guiPreviewPanel = nil

-- 3D Object Preview Export
local previewObject = nil
local previewElement = nil
local previewRotZ = 0.0

-- ====================================================================
-- 1. WEAPON REPLACEMENT: Silenced 9mm (Model ID: 347)
-- ====================================================================
local function loadToolGunWeaponModel()
    if fileExists("toolgun.txd") and fileExists("toolgun.dff") then
        local txd = engineLoadTXD("toolgun.txd")
        if txd then
            engineImportTXD(txd, TOOLGUN_CONFIG.weaponModel)
        end
        local dff = engineLoadDFF("toolgun.dff")
        if dff then
            engineReplaceModel(dff, TOOLGUN_CONFIG.weaponModel)
        end
        outputDebugString("[Tool Gun] Custom weapon model replaced on ID " .. TOOLGUN_CONFIG.weaponModel)
    else
        outputDebugString("[Tool Gun] Default weapon used (toolgun.txd/dff files optional).")
    end
end

-- ====================================================================
-- 2. OBJECT PREVIEW HELPER (Exports: object_preview Integration)
-- ====================================================================
local function destroyActivePreview()
    if previewElement then
        local objPrevRes = getResourceFromName("object_preview")
        if objPrevRes and getResourceState(objPrevRes) == "running" then
            pcall(exports.object_preview.destroyObjectPreview, previewElement)
        end
        previewElement = nil
    end
    if isElement(previewObject) then
        destroyElement(previewObject)
        previewObject = nil
    end
end

local function updateActivePreview(modelId)
    destroyActivePreview()
    if not modelId or type(modelId) ~= "number" then return end

    local objPrevRes = getResourceFromName("object_preview")
    if not (objPrevRes and getResourceState(objPrevRes) == "running") then
        return
    end

    -- Create off-screen dummy object near player so it stays streamed in
    local px, py, pz = getElementPosition(localPlayer)
    previewObject = createObject(modelId, px, py, pz - 15)
    setElementAlpha(previewObject, 255)
    if not isElement(previewObject) then return end
    setElementCollisionsEnabled(previewObject, false)

    -- Get absolute screen coordinates of the preview panel
    if isElement(guiPreviewPanel) then
        local px, py = guiGetPosition(guiPreviewPanel, false)
        local pw, ph = guiGetSize(guiPreviewPanel, false)
        local wx, wy = guiGetPosition(guiWindow, false)
        local absX = wx + px + 2
        local absY = wy + py + 2
        local absW = pw - 4
        local absH = ph - 4

        local success, result = pcall(function()
            return exports.object_preview:createObjectPreview(previewObject, 0, 0, 0, absX, absY, absW, absH, false, true)
        end)
        if success and result then
            previewElement = result
        end
    end
end

-- ====================================================================
-- 3. CEGUI INTERFACE (MTA Default GUI)
-- ====================================================================
local function populateGridList(categoryKey, searchFilter)
    if not isElement(guiGridList) then return end
    guiGridListClear(guiGridList)

    local search = searchFilter and searchFilter:lower() or ""

    if categoryKey == "objects" then
        for _, item in ipairs(TOOLGUN_CATALOG.objects.items) do
            local matchesSearch = (search == "") or (item.name:lower():find(search, 1, true)) or (tostring(item.id):find(search, 1, true))
            if matchesSearch then
                local row = guiGridListAddRow(guiGridList)
                guiGridListSetItemText(guiGridList, row, 1, item.name, false, false)
                guiGridListSetItemText(guiGridList, row, 2, tostring(item.id), false, false)
                guiGridListSetItemData(guiGridList, row, 1, { type = "object", data = item })
            end
        end
    elseif categoryKey == "lights" then
        for _, item in ipairs(TOOLGUN_CATALOG.lights.items) do
            local matchesSearch = (search == "") or (item.name:lower():find(search, 1, true))
            if matchesSearch then
                local row = guiGridListAddRow(guiGridList)
                guiGridListSetItemText(guiGridList, row, 1, item.name, false, false)
                guiGridListSetItemText(guiGridList, row, 2, "LIGHT", false, false)
                guiGridListSetItemData(guiGridList, row, 1, { type = "light", data = item })
            end
        end
    elseif categoryKey == "effects" then
        for _, item in ipairs(TOOLGUN_CATALOG.effects.items) do
            local matchesSearch = (search == "") or (item.name:lower():find(search, 1, true)) or (item.effectName:lower():find(search, 1, true))
            if matchesSearch then
                local row = guiGridListAddRow(guiGridList)
                guiGridListSetItemText(guiGridList, row, 1, item.name, false, false)
                guiGridListSetItemText(guiGridList, row, 2, item.effectName, false, false)
                guiGridListSetItemData(guiGridList, row, 1, { type = "effect", data = item })
            end
        end
    end
end

local function createToolGunGUI()
    if isElement(guiWindow) then return end

    local winW, winH = 720, 500
    local winX = (sw - winW) / 2
    local winY = (sh - winH) / 2

    guiWindow = guiCreateWindow(winX, winY, winW, winH, "Garry's Mod Tool Gun - Prop Spawner & Editor", false)
    guiWindowSetSizable(guiWindow, false)

    -- Category Selector (ComboBox)
    guiCreateLabel(16, 28, 140, 18, "Kategori Secin:", false, guiWindow)
    guiCategoryCombo = guiCreateComboBox(16, 48, 180, 100, "Objeler (Props)", false, guiWindow)
    guiComboBoxAddItem(guiCategoryCombo, "Objeler (Props)")
    guiComboBoxAddItem(guiCategoryCombo, "Işıklar (Point Lights)")
    guiComboBoxAddItem(guiCategoryCombo, "Efektler (Visual FX)")

    -- Search Edit Box
    guiCreateLabel(210, 28, 180, 18, "Filtre / Arama:", false, guiWindow)
    guiSearchBox = guiCreateEdit(210, 48, 200, 24, "", false, guiWindow)

    -- Item GridList
    guiGridList = guiCreateGridList(16, 82, 394, 350, false, guiWindow)
    guiGridListAddColumn(guiGridList, "Nesne / Isim", 0.70)
    guiGridListAddColumn(guiGridList, "Model / ID", 0.25)

    -- 3D Preview Panel Area (Right Side)
    guiCreateLabel(426, 28, 260, 18, "3D Canli Onizleme:", false, guiWindow)
    guiPreviewPanel = guiCreateLabel(426, 48, 276, 280, "", false, guiWindow)

    -- Preview Info Label
    local infoBox = guiCreateMemo(426, 334, 276, 98, "GridList'ten bir nesne secin.\n'Kusan / Spawn' tusuna basip Silenced 9mm ile Sol Tik yaparak dunyaya yerlestirin.", false, guiWindow)
    guiMemoSetReadOnly(infoBox, true)

    -- Action Buttons (Bottom Bar)
    guiBtnSpawn = guiCreateButton(16, 442, 180, 40, "SPAWN / KUSAN", false, guiWindow)
    guiBtnDeleteMode = guiCreateButton(206, 442, 120, 40, "SILME MODU", false, guiWindow)
    guiBtnSave = guiCreateButton(336, 442, 120, 40, "KAYDET", false, guiWindow)
    guiBtnClose = guiCreateButton(572, 442, 130, 40, "KAPAT", false, guiWindow)

    -- Events
    addEventHandler("onClientGUIComboBoxAccepted", guiCategoryCombo, function()
        local selectedIdx = guiComboBoxGetSelected(guiCategoryCombo)
        local catKey = "objects"
        if selectedIdx == 1 then catKey = "lights"
        elseif selectedIdx == 2 then catKey = "effects" end
        populateGridList(catKey, guiGetText(guiSearchBox))
        destroyActivePreview()
    end, false)

    addEventHandler("onClientGUIChanged", guiSearchBox, function()
        local selectedIdx = guiComboBoxGetSelected(guiCategoryCombo)
        local catKey = "objects"
        if selectedIdx == 1 then catKey = "lights"
        elseif selectedIdx == 2 then catKey = "effects" end
        populateGridList(catKey, guiGetText(guiSearchBox))
    end, false)

    addEventHandler("onClientGUIClick", guiGridList, function(button, state)
        if button ~= "left" or state ~= "up" then return end
        local selectedRow = guiGridListGetSelectedItem(guiGridList)
        if selectedRow ~= -1 then
            local data = guiGridListGetItemData(guiGridList, selectedRow, 1)
            if data and data.type == "object" then
                updateActivePreview(data.data.id)
            else
                destroyActivePreview()
            end
        end
    end, false)

    -- "SPAWN / KUŞAN" BUTTON
    addEventHandler("onClientGUIClick", guiBtnSpawn, function(button, state)
        if button ~= "left" or state ~= "up" then return end
        local selectedRow = guiGridListGetSelectedItem(guiGridList)
        if selectedRow == -1 then
            outputChatBox("#FF9900[Tool Gun] #FFFFFFLutfen listeden bir nesne secin!", 255, 255, 255, true)
            return
        end

        local item = guiGridListGetItemData(guiGridList, selectedRow, 1)
        if item then
            currentToolMode = item.type
            currentItemData = item.data

            -- Switch player weapon to Tool Gun (Silenced 9mm)
            if getPedWeapon(localPlayer) ~= TOOLGUN_CONFIG.weaponId then
                setPedWeaponSlot(localPlayer, 2)
            end

            toggleToolGunGUI(false)
            outputChatBox("#00FF88[Tool Gun] #FFFFFFSecilen: #00CCFF" .. item.data.name .. " #FFFFFF| Sol Tik ile yerlestirin!", 255, 255, 255, true)
        end
    end, false)

    addEventHandler("onClientGUIClick", guiBtnSave, function(button, state)
        if button ~= "left" or state ~= "up" then return end
        triggerServerEvent("toolgun:serverExportMap", localPlayer, "toolgun_exported.map")
        outputChatBox("#00FF88[Tool Gun] #FFFFFFHarita kaydediliyor...", 255, 255, 255, true)
    end, false)

    -- "SİLME MODU" BUTTON
    addEventHandler("onClientGUIClick", guiBtnDeleteMode, function(button, state)
        if button ~= "left" or state ~= "up" then return end
        currentToolMode = "delete"
        currentItemData = nil
        toggleToolGunGUI(false)
        outputChatBox("#FF4444[Tool Gun] #FFFFFFSilme Modu Aktif! Yerlestirilen nesnelere ates ederek silebilirsiniz.", 255, 255, 255, true)
    end, false)

    addEventHandler("onClientGUIClick", guiBtnClose, function(button, state)
        if button ~= "left" or state ~= "up" then return end
        toggleToolGunGUI(false)
    end, false)

    -- Initial load
    populateGridList("objects", "")
    
    -- Ctrl+Z Undo
    bindKey("z", "down", function()
        if getKeyState("lctrl") or getKeyState("rctrl") then
            if getPedWeapon(localPlayer) == TOOLGUN_CONFIG.weaponId then
                triggerServerEvent("toolgun:serverUndo", localPlayer)
            end
        end
    end)
    
    -- Weapon Switch Handling (Disable fire/ctrl)
    addEventHandler("onClientPlayerWeaponSwitch", localPlayer, function(prev, cur)
        if getPedWeapon(localPlayer, cur) == TOOLGUN_CONFIG.weaponId then
            toggleControl("fire", false)
            toggleControl("action", false)
        else
            toggleControl("fire", true)
            toggleControl("action", true)
        end
    end)
    guiSetVisible(guiWindow, false)
end

function toggleToolGunGUI(state)
    if state == nil then
        state = not isGuiOpen
    end
    isGuiOpen = state

    if not isElement(guiWindow) then
        createToolGunGUI()
    end

    guiSetVisible(guiWindow, isGuiOpen)
    showCursor(isGuiOpen)

    if not isGuiOpen then
        destroyActivePreview()
    else
        guiBringToFront(guiWindow)
    end
end
bindKey(TOOLGUN_CONFIG.toggleGuiKey, "down", function()
    if getPedWeapon(localPlayer) == TOOLGUN_CONFIG.weaponId then
        toggleToolGunGUI()
    else
        outputChatBox("#FF0000[Tool Gun] #FFFFFFBu menuyu acmak icin elinizde Tool Gun (Silenced 9mm) olmali!", 255, 255, 255, true)
    end
end)
addCommandHandler("spawner", function() toggleToolGunGUI() end)

-- ====================================================================
-- 4. GHOST / PREVIEW CLEANUP HELPER
-- ====================================================================
local function cleanupGhost()
    if isElement(ghostElement) then
        destroyElement(ghostElement)
        ghostElement = nil
    end
    if isElement(ghostLight) then
        destroyElement(ghostLight)
        ghostLight = nil
    end
    if isElement(ghostEffect) then
        destroyElement(ghostEffect)
        ghostEffect = nil
    end
    isEditingGhost = false
end

-- ====================================================================
-- 5. TOOL GUN PLACEMENT & RAYCAST INTERACTION
-- ====================================================================
local function getRaycastTargetPosition()
    local cx, cy, cz, lx, ly, lz = getCameraMatrix()
    local dirX = lx - cx
    local dirY = ly - cy
    local dirZ = lz - cz
    local length = math.sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ)
    if length <= 0 then length = 1 end

    local rayX = cx + (dirX / length) * TOOLGUN_CONFIG.maxReachDistance
    local rayY = cy + (dirY / length) * TOOLGUN_CONFIG.maxReachDistance
    local rayZ = cz + (dirZ / length) * TOOLGUN_CONFIG.maxReachDistance

    local hit, hitX, hitY, hitZ, hitElement = processLineOfSight(
        cx, cy, cz,
        rayX, rayY, rayZ,
        true, true, true, true, true, false, false, false,
        localPlayer
    )

    if hit then
        return hitX, hitY, hitZ, hitElement
    else
        return rayX, rayY, rayZ, nil
    end
end

-- Manual Left-Click Trigger (Replacing WeaponFire to prevent ammo loss and allow cooldown)
local lastFireTime = 0
bindKey("mouse1", "down", function()
    if getPedWeapon(localPlayer) ~= TOOLGUN_CONFIG.weaponId then return end
    
    local now = getTickCount()
    if now - lastFireTime < 500 then return end
    lastFireTime = now
    

    if isGuiOpen or isCursorShowing() then return end

    local targetX, targetY, targetZ, targetElement = getRaycastTargetPosition()



    -- 1. DELETE MODE
    if currentToolMode == "delete" then
        if isElement(targetElement) and getElementData(targetElement, "toolgun:id") then
            triggerServerEvent("toolgun:serverDeleteItem", localPlayer, targetElement)
            playSoundFrontEnd(41)
        else
            outputChatBox("#FF9900[Tool Gun] #FFFFFFHedeflenen nesne Tool Gun ile olusturulmamis!", 255, 255, 255, true)
        end
        return
    end

    -- 2. SPAWN MODE
    if not currentItemData then
        outputChatBox("#FFCC00[Tool Gun] #FFFFFFLutfen once '#00FF88B#FFFFFF' tusuna basip bir nesne veya isik secin!", 255, 255, 255, true)
        return
    end

    cleanupGhost()

    ghostPos.x = targetX
    ghostPos.y = targetY
    ghostPos.z = targetZ
    ghostRot.x = 0
    ghostRot.y = 0
    ghostRot.z = getPedRotation(localPlayer)

    if currentToolMode == "object" then
        ghostElement = createObject(currentItemData.id, ghostPos.x, ghostPos.y, ghostPos.z, ghostRot.x, ghostRot.y, ghostRot.z)
        if isElement(ghostElement) then
            setElementAlpha(ghostElement, TOOLGUN_CONFIG.ghostAlpha)
            setElementCollisionsEnabled(ghostElement, false)
            setElementDimension(ghostElement, getElementDimension(localPlayer))
            setElementInterior(ghostElement, getElementInterior(localPlayer))
            isEditingGhost = true
            playSoundFrontEnd(40)
        end

    elseif currentToolMode == "light" then
        ghostLight = createLight(0, ghostPos.x, ghostPos.y, ghostPos.z, currentItemData.radius or 8.0, currentItemData.r, currentItemData.g, currentItemData.b)
        setElementDimension(ghostLight, getElementDimension(localPlayer))
        setElementInterior(ghostLight, getElementInterior(localPlayer))
        isEditingGhost = true
        playSoundFrontEnd(40)

    elseif currentToolMode == "effect" then
        ghostEffect = createEffect(currentItemData.effectName, ghostPos.x, ghostPos.y, ghostPos.z, ghostRot.x, ghostRot.y, ghostRot.z)
        setElementDimension(ghostEffect, getElementDimension(localPlayer))
        setElementInterior(ghostEffect, getElementInterior(localPlayer))
        isEditingGhost = true
        playSoundFrontEnd(40)
    end
end)

-- ====================================================================
-- 6. GIZMO / EDITING CONTROLS (Arrow Keys, PgUp/PgDn, Mouse Scroll)
-- ====================================================================

-- Precision Movement: Arrow Keys (X and Y relative to camera)
local function moveGhostRelative(deltaFwd, deltaRight)
    if not isEditingGhost then return end

    local _, _, _, lx, ly, _ = getCameraMatrix()
    local px, py, _ = getElementPosition(localPlayer)
    local angle = math.atan2(ly - py, lx - px)

    local fwdX = math.cos(angle)
    local fwdY = math.sin(angle)
    local rightX = math.sin(angle)
    local rightY = -math.cos(angle)

    ghostPos.x = ghostPos.x + fwdX * deltaFwd + rightX * deltaRight
    ghostPos.y = ghostPos.y + fwdY * deltaFwd + rightY * deltaRight

    if isElement(ghostElement) then setElementPosition(ghostElement, ghostPos.x, ghostPos.y, ghostPos.z) end
    if isElement(ghostLight) then setElementPosition(ghostLight, ghostPos.x, ghostPos.y, ghostPos.z) end
    if isElement(ghostEffect) then setElementPosition(ghostEffect, ghostPos.x, ghostPos.y, ghostPos.z) end
end

bindKey("arrow_u", "down", function() if getPedWeapon(localPlayer) == TOOLGUN_CONFIG.weaponId then moveGhostRelative(TOOLGUN_CONFIG.stepMovement, 0) end end)
bindKey("arrow_d", "down", function() if getPedWeapon(localPlayer) == TOOLGUN_CONFIG.weaponId then moveGhostRelative(-TOOLGUN_CONFIG.stepMovement, 0) end end)
bindKey("arrow_l", "down", function() if getPedWeapon(localPlayer) == TOOLGUN_CONFIG.weaponId then moveGhostRelative(0, -TOOLGUN_CONFIG.stepMovement) end end)
bindKey("arrow_r", "down", function() if getPedWeapon(localPlayer) == TOOLGUN_CONFIG.weaponId then moveGhostRelative(0, TOOLGUN_CONFIG.stepMovement) end end)

-- Vertical Movement: PgUp / PgDn
bindKey("pgup", "down", function()
    if not isEditingGhost then return end
    ghostPos.z = ghostPos.z + TOOLGUN_CONFIG.stepMovement
    if isElement(ghostElement) then setElementPosition(ghostElement, ghostPos.x, ghostPos.y, ghostPos.z) end
    if isElement(ghostLight) then setElementPosition(ghostLight, ghostPos.x, ghostPos.y, ghostPos.z) end
    if isElement(ghostEffect) then setElementPosition(ghostEffect, ghostPos.x, ghostPos.y, ghostPos.z) end
end)

bindKey("pgdn", "down", function()
    if not isEditingGhost then return end
    ghostPos.z = ghostPos.z - TOOLGUN_CONFIG.stepMovement
    if isElement(ghostElement) then setElementPosition(ghostElement, ghostPos.x, ghostPos.y, ghostPos.z) end
    if isElement(ghostLight) then setElementPosition(ghostLight, ghostPos.x, ghostPos.y, ghostPos.z) end
    if isElement(ghostEffect) then setElementPosition(ghostEffect, ghostPos.x, ghostPos.y, ghostPos.z) end
end)

-- Rotation: Mouse Wheel Up / Down
bindKey("mouse_wheel_up", "down", function()
    if not isEditingGhost then return end
    ghostRot.z = (ghostRot.z + TOOLGUN_CONFIG.stepRotation) % 360
    if isElement(ghostElement) then setElementRotation(ghostElement, ghostRot.x, ghostRot.y, ghostRot.z) end
    if isElement(ghostEffect) then setElementRotation(ghostEffect, ghostRot.x, ghostRot.y, ghostRot.z) end
end)

bindKey("mouse_wheel_down", "down", function()
    if not isEditingGhost then return end
    ghostRot.z = (ghostRot.z - TOOLGUN_CONFIG.stepRotation) % 360
    if isElement(ghostElement) then setElementRotation(ghostElement, ghostRot.x, ghostRot.y, ghostRot.z) end
    if isElement(ghostEffect) then setElementRotation(ghostEffect, ghostRot.x, ghostRot.y, ghostRot.z) end
end)

-- Confirm & Freeze: Right Click (Mouse 2)
bindKey("mouse2", "down", function()
    if not isEditingGhost then return end
    if getPedWeapon(localPlayer) ~= TOOLGUN_CONFIG.weaponId then return end
    if isGuiOpen or isCursorShowing() then return end

    local extra = {}
    local idOrName = nil

    if currentToolMode == "object" and currentItemData then
        idOrName = currentItemData.id
    elseif currentToolMode == "light" and currentItemData then
        idOrName = currentItemData.name
        extra.r = currentItemData.r
        extra.g = currentItemData.g
        extra.b = currentItemData.b
        extra.radius = currentItemData.radius
    elseif currentToolMode == "effect" and currentItemData then
        idOrName = currentItemData.effectName
    end

    if idOrName then
        triggerServerEvent("toolgun:serverPlaceItem", localPlayer, currentToolMode, idOrName, ghostPos.x, ghostPos.y, ghostPos.z, ghostRot.x, ghostRot.y, ghostRot.z, extra)
        playSoundFrontEnd(40)
    end

    cleanupGhost()
end)

-- Delete Key: Cancel Ghost or Delete Target
bindKey(TOOLGUN_CONFIG.deleteKey, "down", function()
    if isEditingGhost then
        cleanupGhost()
        outputChatBox("#FF9900[Tool Gun] #FFFFFFYerlestirme iptal edildi.", 255, 255, 255, true)
    else
        local _, _, _, targetElement = getRaycastTargetPosition()
        if isElement(targetElement) and getElementData(targetElement, "toolgun:id") then
            triggerServerEvent("toolgun:serverDeleteItem", localPlayer, targetElement)
            playSoundFrontEnd(41)
        end
    end
end)

-- ====================================================================
-- 7. 3D GIZMO & HUD ON-SCREEN RENDERING
-- ====================================================================
addEventHandler("onClientRender", root, function()
    -- Draw 3D GUI Preview Background if GUI is open
    if isGuiOpen and isElement(guiWindow) and isElement(guiPreviewPanel) then
        local px, py = guiGetPosition(guiPreviewPanel, false)
        local pw, ph = guiGetSize(guiPreviewPanel, false)
        local wx, wy = guiGetPosition(guiWindow, false)
        local absX = wx + px
        local absY = wy + py
        dxDrawRectangle(absX, absY, pw, ph, tocolor(20, 26, 35, 240), true)
        dxDrawRectangle(absX, absY, pw, 2, tocolor(0, 200, 255, 230), true)
        dxDrawRectangle(absX, absY + ph - 2, pw, 2, tocolor(0, 200, 255, 120), true)
        if not previewElement then
            dxDrawText("3D Canli Onizleme Icin\nObjelerden Birini Secin", absX, absY, absX + pw, absY + ph, tocolor(120, 140, 165, 200), 1.0, "default-bold", "center", "center", false, false, true)
        end
    end

    -- Rotate 3D GUI preview if active
    if previewElement then
        previewRotZ = (previewRotZ + 0.8) % 360
        local objPrevRes = getResourceFromName("object_preview")
        if objPrevRes and getResourceState(objPrevRes) == "running" then
            pcall(exports.object_preview.setRotation, previewElement, 0, 0, previewRotZ)
        end
    end

    -- Tool Gun In-Hand Check
    local isHoldingToolGun = (getPedWeapon(localPlayer) == TOOLGUN_CONFIG.weaponId)
    if not isHoldingToolGun then
        if isEditingGhost then cleanupGhost() end
        return
    end

    -- 3D GIZMO LINES AROUND GHOST
    if isEditingGhost then
        local gx, gy, gz = ghostPos.x, ghostPos.y, ghostPos.z
        -- X-axis (Red)
        dxDrawLine3D(gx, gy, gz, gx + 0.75, gy, gz, tocolor(255, 40, 40, 220), 2.5)
        -- Y-axis (Green)
        dxDrawLine3D(gx, gy, gz, gx, gy + 0.75, gz, tocolor(40, 255, 40, 220), 2.5)
        -- Z-axis (Blue)
        dxDrawLine3D(gx, gy, gz, gx, gy, gz + 0.75, tocolor(40, 140, 255, 220), 2.5)

        -- 3D Text above ghost
        local sx, sy = getScreenFromWorldPosition(gx, gy, gz + 0.85)
        if sx and sy then
            local infoText = string.format("X: %.1f  Y: %.1f  Z: %.1f | RotZ: %.0f°", gx, gy, gz, ghostRot.z)
            dxDrawText(infoText, sx + 1, sy + 1, sx + 1, sy + 1, tocolor(0, 0, 0, 240), 1.0, "default-bold", "center", "center")
            dxDrawText(infoText, sx, sy, sx, sy, tocolor(0, 255, 180, 240), 1.0, "default-bold", "center", "center")
        end
    end

    -- FLOATING GARRYS MOD STYLE HUD (Bottom-Center)
    if not isGuiOpen then
        local hudW, hudH = 460, 68
        local hudX = (sw - hudW) / 2
        local hudY = sh - 85

        -- Background Frame
        dxDrawRectangle(hudX, hudY, hudW, hudH, tocolor(15, 20, 26, 210), false)
        dxDrawRectangle(hudX, hudY, hudW, 2, tocolor(0, 200, 255, 230), false)

        -- Tool Mode & Selected Prop Header
        local modeTitle = "#00CCFFMOD: #FFFFFF" .. (currentToolMode:upper())
        if currentToolMode == "delete" then
            modeTitle = "#FF3333MOD: #FFFFFFSILME ARACI (DELETE TOOL)"
        elseif currentItemData then
            modeTitle = modeTitle .. "  |  #00FF88" .. currentItemData.name
        else
            modeTitle = modeTitle .. "  |  #AAAAAA[B Tusuna Basip Nesne Secin]"
        end

        dxDrawText(modeTitle, hudX + 12, hudY + 8, hudX + hudW - 12, hudY + 28, tocolor(255, 255, 255, 255), 0.95, "default-bold", "left", "top", false, false, false, true)

        -- Controls Cheatsheet
        local cheatsheet = "[Sol Tik]: Konumlandir   [Sag Tik]: ONAYLA & DONDUR\n[Yon Tuslari]: X/Y Kaydir   [PgUp/PgDn]: Z Yukseklik   [Scroll]: Dondur   [Del]: Sil"
        if currentToolMode == "delete" then
            cheatsheet = "[Sol Tik]: Bakilan Nesneyi Sil   [B Tusu]: Nesne Secim Menusu"
        end
        dxDrawText(cheatsheet, hudX + 12, hudY + 30, hudX + hudW - 12, hudY + hudH - 6, tocolor(190, 205, 225, 210), 0.80, "default", "left", "top", false, false, false, false)
    end
end)

-- ====================================================================
-- 8. CLIENT-SIDE LIGHTS & EFFECTS SYNCHRONIZATION
-- ====================================================================
addEvent("toolgun:clientReceiveInitialSync", true)
addEventHandler("toolgun:clientReceiveInitialSync", root, function(lights, effects)
    -- Clear previous
    for id, l in pairs(activeClientLights) do if isElement(l) then destroyElement(l) end end
    for id, e in pairs(activeClientEffects) do if isElement(e) then destroyElement(e) end end
    activeClientLights = {}
    activeClientEffects = {}

    for id, data in pairs(lights or {}) do
        activeClientLights[id] = createLight(0, data.x, data.y, data.z, data.radius or 8.0, data.r, data.g, data.b)
    end
    for id, data in pairs(effects or {}) do
        activeClientEffects[id] = createEffect(data.effectName, data.x, data.y, data.z, data.rx or 0, data.ry or 0, data.rz or 0)
    end
end)

addEvent("toolgun:clientSyncLightAdd", true)
addEventHandler("toolgun:clientSyncLightAdd", root, function(id, data)
    if activeClientLights[id] and isElement(activeClientLights[id]) then
        destroyElement(activeClientLights[id])
    end
    activeClientLights[id] = createLight(0, data.x, data.y, data.z, data.radius or 8.0, data.r, data.g, data.b)
end)

addEvent("toolgun:clientSyncLightRemove", true)
addEventHandler("toolgun:clientSyncLightRemove", root, function(id)
    if activeClientLights[id] and isElement(activeClientLights[id]) then
        destroyElement(activeClientLights[id])
        activeClientLights[id] = nil
    end
end)

addEvent("toolgun:clientSyncEffectAdd", true)
addEventHandler("toolgun:clientSyncEffectAdd", root, function(id, data)
    if activeClientEffects[id] and isElement(activeClientEffects[id]) then
        destroyElement(activeClientEffects[id])
    end
    activeClientEffects[id] = createEffect(data.effectName, data.x, data.y, data.z, data.rx or 0, data.ry or 0, data.rz or 0)
end)

addEvent("toolgun:clientSyncEffectRemove", true)
addEventHandler("toolgun:clientSyncEffectRemove", root, function(id)
    if activeClientEffects[id] and isElement(activeClientEffects[id]) then
        destroyElement(activeClientEffects[id])
        activeClientEffects[id] = nil
    end
end)

-- ====================================================================
-- 9. RESOURCE LIFECYCLE
-- ====================================================================
addEventHandler("onClientResourceStart", resourceRoot, function()
    loadToolGunWeaponModel()
    createToolGunGUI()
    triggerServerEvent("toolgun:requestInitialSync", localPlayer)
    outputChatBox("#00FF88[Tool Gun] #FFFFFFGarry's Mod Tool Gun yuklendi! Menuyu acmak icin '#00FF88B#FFFFFF' tusuna basin.", 255, 255, 255, true)
end)

addEventHandler("onClientResourceStop", resourceRoot, function()
    cleanupGhost()
    destroyActivePreview()
    for id, l in pairs(activeClientLights) do if isElement(l) then destroyElement(l) end end
    for id, e in pairs(activeClientEffects) do if isElement(e) then destroyElement(e) end end
    if isElement(guiWindow) then destroyElement(guiWindow) end
    showCursor(false)
end)
