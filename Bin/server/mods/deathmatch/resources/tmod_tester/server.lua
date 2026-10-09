-- ====================================================================
-- TMOD TESTER - SUNUCU TARAFI
-- ====================================================================

addEvent("tmod:serverSpawnVehicle", true)
addEventHandler("tmod:serverSpawnVehicle", root, function(modelId)
    local x, y, z = getElementPosition(client)
    local rx, ry, rz = getElementRotation(client)
    local veh = createVehicle(modelId, x + 3, y + 3, z + 0.5, 0, 0, rz)
    if veh then
        outputChatBox(string.format("[TMOD::SRV] Vehicle entity spawned [Model: %d, Origin: {%.1f, %.1f, %.1f}]", modelId, x + 3, y + 3, z + 0.5), client, 120, 175, 135)
    end
end)

addEvent("tmod:serverHealPlayer", true)
addEventHandler("tmod:serverHealPlayer", root, function()
    setElementHealth(client, 100)
    setPedArmor(client, 100)
    outputChatBox("[TMOD::SRV] Entity status restored: Health=100.0, Armor=100.0", client, 135, 170, 195)
end)
