import os

client_path = r'c:\Users\dogu\Desktop\server\mods\deathmatch\resources\toolgun\client.lua'
server_path = r'c:\Users\dogu\Desktop\server\mods\deathmatch\resources\toolgun\server.lua'

with open(client_path, 'r', encoding='utf-8') as f:
    c = f.read()

with open(server_path, 'r', encoding='utf-8') as f:
    s = f.read()

# 1. Modify Server Undo & Export
s = s.replace(
'''addCommandHandler("undo", function(player)
    if not isElement(player) then return end''',
'''addEvent("toolgun:serverUndo", true)
addEventHandler("toolgun:serverUndo", root, function()
    local player = client
    if not isElement(player) then return end'''
)

s = s.replace(
'''addCommandHandler("exportmap", function(player, cmd, filename)''',
'''addEvent("toolgun:serverExportMap", true)
addEventHandler("toolgun:serverExportMap", root, function(filename)
    local player = client
'''
)

with open(server_path, 'w', encoding='utf-8') as f:
    f.write(s)


# 2. Modify Client CEGUI (Save button & Fix object_preview destruction)

# Destroy fix: The user says old preview stays. Let's make sure we destroy BOTH explicitly
old_destroy = '''local function destroyActivePreview()
    if previewElement then
        local objPrevRes = getResourceFromName("object_preview")
        if objPrevRes and getResourceState(objPrevRes) == "running" then
            pcall(function() exports.object_preview:destroyObjectPreview(previewElement) end)
        end
        previewElement = nil
    end
    if isElement(previewObject) then
        destroyElement(previewObject)
        previewObject = nil
    end
end'''

new_destroy = '''local function destroyActivePreview()
    if previewElement then
        pcall(function() exports.object_preview:destroyObjectPreview(previewElement) end)
        previewElement = nil
    end
    if isElement(previewObject) then
        pcall(function() exports.object_preview:destroyObjectPreview(previewObject) end)
        destroyElement(previewObject)
        previewObject = nil
    end
end'''
c = c.replace(old_destroy, new_destroy)

# Add Save Button
c = c.replace(
    'guiBtnDeleteMode = guiCreateButton(206, 442, 150, 40, "SILME MODU", false, guiWindow)',
    'guiBtnDeleteMode = guiCreateButton(206, 442, 120, 40, "SILME MODU", false, guiWindow)\n    guiBtnSave = guiCreateButton(336, 442, 120, 40, "KAYDET", false, guiWindow)'
)

c = c.replace(
    '-- "SİLME MODU" BUTTON',
    '''addEventHandler("onClientGUIClick", guiBtnSave, function(button, state)
        if button ~= "left" or state ~= "up" then return end
        triggerServerEvent("toolgun:serverExportMap", localPlayer, "toolgun_exported.map")
        outputChatBox("#00FF88[Tool Gun] #FFFFFFHarita kaydediliyor...", 255, 255, 255, true)
    end, false)

    -- "SİLME MODU" BUTTON'''
)


# 3. Fix Cooldown, Ctrl Disable, and Mouse1 Binding instead of WeaponFire
c = c.replace(
    '''-- Weapon Fire / Left-Click Trigger
addEventHandler("onClientPlayerWeaponFire", localPlayer, function(weapon, ammo, ammoInClip, hitX, hitY, hitZ, hitElement)
    if weapon ~= TOOLGUN_CONFIG.weaponId then return end''',
    '''-- Manual Left-Click Trigger (Replacing WeaponFire to prevent ammo loss and allow cooldown)
local lastFireTime = 0
bindKey("mouse1", "down", function()
    if getPedWeapon(localPlayer) ~= TOOLGUN_CONFIG.weaponId then return end
    
    local now = getTickCount()
    if now - lastFireTime < 500 then return end
    lastFireTime = now
    
    local targetX, targetY, targetZ, targetElement = getRaycastTargetPosition()'''
)

c = c.replace(
    '''    local targetX, targetY, targetZ, targetElement = getRaycastTargetPosition()''',
    ''
) # Wait, doing it like this will remove it completely if I replace it with empty.
# Let's use string operations safely.

c = c.replace(
    '''-- Initial load
    populateGridList("objects", "")''',
    '''-- Initial load
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
    end)'''
)

# For the raycast removal:
c = c.replace('''local targetX, targetY, targetZ, targetElement = getRaycastTargetPosition()
    
    local targetX, targetY, targetZ, targetElement = getRaycastTargetPosition()''', '''local targetX, targetY, targetZ, targetElement = getRaycastTargetPosition()''')


# Arrow keys fix
c = c.replace(
'''bindKey("arrow_u", "down", function() moveGhostRelative(TOOLGUN_CONFIG.stepMovement, 0) end)
bindKey("arrow_d", "down", function() moveGhostRelative(-TOOLGUN_CONFIG.stepMovement, 0) end)
bindKey("arrow_l", "down", function() moveGhostRelative(0, -TOOLGUN_CONFIG.stepMovement) end)
bindKey("arrow_r", "down", function() moveGhostRelative(0, TOOLGUN_CONFIG.stepMovement) end)''',
'''bindKey("arrow_u", "down", function() if getPedWeapon(localPlayer) == TOOLGUN_CONFIG.weaponId then moveGhostRelative(TOOLGUN_CONFIG.stepMovement, 0) end end)
bindKey("arrow_d", "down", function() if getPedWeapon(localPlayer) == TOOLGUN_CONFIG.weaponId then moveGhostRelative(-TOOLGUN_CONFIG.stepMovement, 0) end end)
bindKey("arrow_l", "down", function() if getPedWeapon(localPlayer) == TOOLGUN_CONFIG.weaponId then moveGhostRelative(0, -TOOLGUN_CONFIG.stepMovement) end end)
bindKey("arrow_r", "down", function() if getPedWeapon(localPlayer) == TOOLGUN_CONFIG.weaponId then moveGhostRelative(0, TOOLGUN_CONFIG.stepMovement) end end)'''
)

with open(client_path, 'w', encoding='utf-8') as f:
    f.write(c)

print('Success')
