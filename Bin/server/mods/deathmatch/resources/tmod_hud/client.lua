--[[
    TMod - Advanced Garry's Mod / Source Engine Style HUD (Fixed Z-Buffer Glitch)
]]

local screenW, screenH = guiGetScreenSize()
local scale = math.min(math.max(screenH / 1080, 0.6), 2)

local bHudVisible = true
local bShowFps = false
local bGodMode = false
local bBuddhaMode = false

local frameCount = 0
local lastFpsTime = getTickCount()
local currentFps = 60

local defaultHudComponents = {
    "ammo", "armour", "breath", "clock", "health", "money", 
    "weapon", "wanted", "radar", "area_name", "vehicle_name"
}

local function setGtaHudHidden(bHide)
    for _, comp in ipairs(defaultHudComponents) do
        setPlayerHudComponentVisible(comp, not bHide)
    end
    setPlayerHudComponentVisible("crosshair", true)
end

-- Z-Buffer bozmayan, postGUI kilitli pürüzsüz GMod kutusu
local function drawSmoothGModBox(x, y, w, h)
    local bgColor = tocolor(35, 40, 48, 130)
    local r = math.floor(8 * scale)

    -- postGUI = true (8. parametre) eklenerek 3D sahneye sızması engellendi
    dxDrawRectangle(x + r, y, w - (r * 2), h, bgColor, true)
    dxDrawRectangle(x, y + r, r, h - (r * 2), bgColor, true)
    dxDrawRectangle(x + w - r, y + r, r, h - (r * 2), bgColor, true)

    for i = 1, r do
        local dy = i - 0.5
        local val = (r * r) - (dy * dy)
        if val > 0 then
            local stripWidth = math.floor(math.sqrt(val))
            dxDrawRectangle(x + r - stripWidth, y + r - i, stripWidth, 1, bgColor, true)
            dxDrawRectangle(x + w - r, y + r - i, stripWidth, 1, bgColor, true)
            dxDrawRectangle(x + r - stripWidth, y + h - r + i - 1, stripWidth, 1, bgColor, true)
            dxDrawRectangle(x + w - r, y + h - r + i - 1, stripWidth, 1, bgColor, true)
        end
    end
end

addEventHandler("onClientRender", root, function()
    -- 1. FPS Göstergesi
    if bShowFps then
        frameCount = frameCount + 1
        local now = getTickCount()
        if (now - lastFpsTime) >= 500 then
            currentFps = math.floor((frameCount * 1000) / (now - lastFpsTime))
            frameCount = 0
            lastFpsTime = now
        end

        local fpsW, fpsH = 100 * scale, 32 * scale
        local fpsX, fpsY = screenW - fpsW - (25 * scale), 20 * scale

        drawSmoothGModBox(fpsX, fpsY, fpsW, fpsH)
        dxDrawText(currentFps .. " fps", fpsX, fpsY, fpsX + fpsW, fpsY + fpsH, tocolor(235, 210, 36, 240), 1.2 * scale, "clear", "center", "center", false, false, true, false, true)
    end

    -- 2. TMod HUD
    if not bHudVisible or isPlayerMapVisible() then return end

    local health = math.max(0, math.floor(getElementHealth(localPlayer)))
    local amberColor = tocolor(235, 210, 36, 255)
    local labelColor = tocolor(235, 210, 36, 210)

    local boxH = 55 * scale
    local bottomMargin = 35 * scale
    local boxY = screenH - bottomMargin - boxH

    -- HEALTH (Sağlık) Kutusu
    local hBoxW = 160 * scale
    local hBoxX = 40 * scale
    drawSmoothGModBox(hBoxX, boxY, hBoxW, boxH)
    
    -- Sondan üçüncü parametre postGUI = true olarak ayarlandı
    dxDrawText("HEALTH", hBoxX + (15 * scale), boxY + (24 * scale), hBoxX + (70 * scale), boxY + (48 * scale), labelColor, 1.1 * scale, "clear", "left", "center", false, false, true, false, true)
    dxDrawText(tostring(health), hBoxX + (60 * scale), boxY, hBoxX + (145 * scale), boxY + boxH, amberColor, 2.8 * scale, "clear", "right", "center", false, false, true, false, true)

    -- AMMO (Mermi) Kutusu
    local currentWeapon = getPedWeapon(localPlayer)
    local weaponSlot = getPedWeaponSlot(localPlayer)
    local totalAmmo = getPedTotalAmmo(localPlayer)
    local clipAmmo = getPedAmmoInClip(localPlayer)

    local bHasAmmoWeapon = (currentWeapon > 1) and (totalAmmo > 0) and (weaponSlot ~= 0) and (weaponSlot ~= 1) and (weaponSlot ~= 10) and (weaponSlot ~= 11)

    if bHasAmmoWeapon then
        local aBoxW = 200 * scale
        local aBoxX = screenW - (40 * scale) - aBoxW
        drawSmoothGModBox(aBoxX, boxY, aBoxW, boxH)

        local reserveAmmo = totalAmmo - clipAmmo
        dxDrawText("AMMO", aBoxX + (15 * scale), boxY + (24 * scale), aBoxX + (70 * scale), boxY + (48 * scale), labelColor, 1.1 * scale, "clear", "left", "center", false, false, true, false, true)

        if totalAmmo == clipAmmo or clipAmmo <= 0 then
            dxDrawText(tostring(totalAmmo), aBoxX + (70 * scale), boxY, aBoxX + (185 * scale), boxY + boxH, amberColor, 2.8 * scale, "clear", "right", "center", false, false, true, false, true)
        else
            dxDrawText(tostring(clipAmmo), aBoxX + (60 * scale), boxY, aBoxX + (130 * scale), boxY + boxH, amberColor, 2.8 * scale, "clear", "right", "center", false, false, true, false, true)
            dxDrawText(tostring(reserveAmmo), aBoxX + (140 * scale), boxY + (14 * scale), aBoxX + (190 * scale), boxY + boxH, tocolor(235, 210, 36, 190), 1.5 * scale, "clear", "left", "center", false, false, true, false, true)
        end
    end
end)

addEventHandler("onClientPreRender", root, function()
    if bGodMode and not isPedDead(localPlayer) then
        if getElementHealth(localPlayer) < 100 then
            setElementHealth(localPlayer, 100)
        end
    end
end)

addEventHandler("onClientPlayerDamage", localPlayer, function(attacker, weapon, bodypart, loss)
    if bGodMode then
        cancelEvent()
        return
    end
    if bBuddhaMode then
        local currentHealth = getElementHealth(localPlayer)
        if (currentHealth - loss) <= 1 then
            cancelEvent()
            setElementHealth(localPlayer, 1)
        end
    end
end)

addCommandHandler("hud", function()
    bHudVisible = not bHudVisible
    setGtaHudHidden(bHudVisible)
    if bHudVisible then
        outputChatBox("[TMod] GMod Source HUD aktif edildi.", 235, 210, 36)
    else
        outputChatBox("[TMod] GTA:SA varsayilan HUD aktif edildi.", 200, 200, 200)
    end
end)

addCommandHandler("kill", function()
    if isPedDead(localPlayer) then return end
    setElementHealth(localPlayer, 0)
    outputChatBox("[TMod] Intihar ettiniz.", 220, 60, 60)
end)

addCommandHandler("showfps", function(cmd, arg)
    if arg == "1" then bShowFps = true
    elseif arg == "0" then bShowFps = false
    else bShowFps = not bShowFps end
    outputChatBox("[TMod] showfps: " .. (bShowFps and "1 (Acik)" or "0 (Kapali)"), 235, 210, 36)
end)

addCommandHandler("god", function()
    bGodMode = not bGodMode
    if bGodMode then
        bBuddhaMode = false
        setElementHealth(localPlayer, 100)
        outputChatBox("[TMod] god modu ACIK (Hasar almazsiniz)", 60, 220, 60)
    else
        outputChatBox("[TMod] god modu KAPALI", 220, 60, 60)
    end
end)

addCommandHandler("buddha", function()
    bBuddhaMode = not bBuddhaMode
    if bBuddhaMode then
        bGodMode = false
        outputChatBox("[TMod] buddha modu ACIK (Caniniz 1'in altina inmez)", 60, 220, 60)
    else
        outputChatBox("[TMod] buddha modu KAPALI", 220, 60, 60)
    end
end)

addEventHandler("onClientResourceStart", resourceRoot, function()
    setGtaHudHidden(true)
    outputChatBox("[TMod] Garry's Mod HUD yuklendi. '/hud' komutu ile degistirebilirsiniz.", 235, 210, 36)
end)

addEventHandler("onClientResourceStop", resourceRoot, function()
    setGtaHudHidden(false)
end)