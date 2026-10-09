-- ====================================================================
-- ÖZEL OBJE FİZİK SİSTEMİ (GMod Sandbox Mantığı + UNREAL ENGINE SIVI FİZİĞİ)
-- ====================================================================

local CONFIG = {
    TOP_ID = 2114, -- GTA'nın kendi Basketbol Topu
    KUP_ID = 1220, -- GTA'nın kendi Ahşap Kutusu
    
    GRAVITY = 0.0055, -- Biraz azaltıldı (Eski: 0.007)
    OBJ_SCALE = 1.5,
    OBJ_RADIUS = 0.42,
}

local physicsObjects = {}
local waterShader = nil

-- ====================================================================
-- YÜKLEME VE SHADER KURULUMU
-- ====================================================================
addEventHandler("onClientResourceStart", resourceRoot, function()
    if fileExists("water.fx") then
        waterShader = dxCreateShader("water.fx", 1, 0, false, "all")
        if waterShader then
            local tex1 = dxCreateTexture("cube_env256.dds")
            local tex2 = dxCreateTexture("smallnoise3d.dds")
            if tex1 then dxSetShaderValue(waterShader, "sReflectionTexture", tex1) end
            if tex2 then dxSetShaderValue(waterShader, "sNoiseTexture", tex2) end
        end
    end
    
    outputChatBox("[DEBUG] targuts-mod: Physics engine initialized. [SUCCESS]", 0, 255, 0)
end)

-- ====================================================================
-- SPAWN KOMUTLARI
-- ====================================================================
local function spawnObject(typeStr, modelId)
    local px, py, pz = getElementPosition(localPlayer)
    local _, _, rotZ = getElementRotation(localPlayer)
    
    local rad = math.rad(rotZ)
    local spawnX = px - math.sin(rad) * 2.0
    local spawnY = py + math.cos(rad) * 2.0
    local spawnZ = pz + 1.5 
    
    local obj = createObject(modelId, spawnX, spawnY, spawnZ)
    setElementCollisionsEnabled(obj, false) 
    setObjectScale(obj, CONFIG.OBJ_SCALE)
    
    if typeStr == "water" and waterShader then
        engineApplyShaderToWorldTexture(waterShader, "*", obj)
    end
    
    table.insert(physicsObjects, {
        element = obj,
        type = typeStr,
        vx = 0, vy = 0, vz = 0,
        rx = 0, ry = 0, rz = rotZ,
        scale = CONFIG.OBJ_SCALE
    })
end

addCommandHandler("spawntop", function() spawnObject("ball", CONFIG.TOP_ID); outputChatBox("[DEBUG] targuts-mod: Spawned entity_ball. [SUCCESS]", 0, 255, 0) end)
addCommandHandler("spawnkup", function() spawnObject("box", CONFIG.KUP_ID); outputChatBox("[DEBUG] targuts-mod: Spawned entity_box. [SUCCESS]", 0, 255, 0) end)
addCommandHandler("spawnwater", function() spawnObject("water", CONFIG.TOP_ID); outputChatBox("[DEBUG] targuts-mod: Spawned entity_fluid. [SUCCESS]", 0, 255, 0) end)

-- ====================================================================
-- MERMİ İTME (Sanal Kesişim)
-- ====================================================================
local function getDistancePointToSegment(px, py, pz, ax, ay, az, bx, by, bz)
    local abX, abY, abZ = bx - ax, by - ay, bz - az
    local apX, apY, apZ = px - ax, py - ay, pz - az
    local abLengthSq = abX^2 + abY^2 + abZ^2
    if abLengthSq == 0 then return math.sqrt(apX^2 + apY^2 + apZ^2) end
    local t = math.max(0, math.min(1, (apX * abX + apY * abY + apZ * abZ) / abLengthSq))
    return math.sqrt((px - (ax + t * abX))^2 + (py - (ay + t * abY))^2 + (pz - (az + t * abZ))^2)
end

addEventHandler("onClientPlayerWeaponFire", localPlayer, function(weapon, ammo, ammoInClip, hitX, hitY, hitZ)
    local wX, wY, wZ = getPedWeaponMuzzlePosition(localPlayer)
    
    for i, objData in ipairs(physicsObjects) do
        if isElement(objData.element) then
            local px, py, pz = getElementPosition(objData.element)
            local dist = getDistancePointToSegment(px, py, pz, wX, wY, wZ, hitX, hitY, hitZ)
            
            local currentRadius = (CONFIG.OBJ_RADIUS / CONFIG.OBJ_SCALE) * objData.scale
            if dist < currentRadius + 0.3 then
                local dirX, dirY, dirZ = hitX - wX, hitY - wY, hitZ - wZ
                local length = math.sqrt(dirX^2 + dirY^2 + dirZ^2)
                
                if length > 0 then
                    objData.vx = objData.vx + (dirX / length) * 0.15
                    objData.vy = objData.vy + (dirY / length) * 0.15
                    objData.vz = objData.vz + (dirZ / length) * 0.15 + 0.05
                    
                    if objData.type == "water" then
                        fxAddWaterSplash(px, py, pz)
                    end
                end
            end
        end
    end
end)

-- ====================================================================
-- FİZİK MOTORU VE UNREAL TARZI SIVI PARÇALANMASI (LIQUID SHATTERING)
-- ====================================================================
addEventHandler("onClientPreRender", root, function(dt)
    local timeScale = dt / 16.666
    local gravity = CONFIG.GRAVITY * timeScale
    
    local ppx, ppy, ppz = getElementPosition(localPlayer)
    local pvx, pvy, pvz = getElementVelocity(localPlayer)
    local playerSpeed = math.sqrt(pvx^2 + pvy^2)
    
    for i = #physicsObjects, 1, -1 do
        local objData = physicsObjects[i]
        
        if isElement(objData.element) then
            
            -- Hızlı Buharlaşma (Su damlaları için)
            if objData.type == "water_drop" then
                objData.scale = objData.scale * (1 - 0.03 * timeScale) -- Sürekli küçülür
                setObjectScale(objData.element, objData.scale)
            end
            
            -- Yok olma kontrolü
            local minScale = (objData.type == "water_drop") and 0.05 or 0.25
            if (objData.type == "water" or objData.type == "water_drop") and objData.scale < minScale then
                destroyElement(objData.element)
                table.remove(physicsObjects, i)
            else
                local px, py, pz = getElementPosition(objData.element)
                
                local isHeldByGravityGun = isElement(getElementData(objData.element, "ggun_taker"))
                
                -- Gravity Gun fırlattığında veya dış etkenlerde (Sadece tutulmuyorken gücü yakala)
                if not isHeldByGravityGun then
                    local evx, evy, evz = getElementVelocity(objData.element)
                    if math.abs(evx) > 0.05 or math.abs(evy) > 0.05 or math.abs(evz) > 0.05 then
                        -- Gravity Gun'ın verdiği devasa fırlatma gücünü biraz yumuşat (Yoksa mermi gibi uçar)
                        objData.vx = evx * 0.3
                        objData.vy = evy * 0.3
                        objData.vz = evz * 0.3
                        setElementVelocity(objData.element, 0, 0, 0)
                    end
                end
                
                if isHeldByGravityGun then
                    -- Objeyi yerçekimi silahı tutuyor! Gravity Gun'ın verdiği hızı (Velocity) biz manuel olarak pozisyona çeviriyoruz!
                    local evx, evy, evz = getElementVelocity(objData.element)
                    px = px + evx
                    py = py + evy
                    pz = pz + evz
                    
                    objData.vx = 0
                    objData.vy = 0
                    objData.vz = 0
                    
                    setElementPosition(objData.element, px, py, pz)
                else
                    -- Normal Fizik Hesaplamaları (Yerçekimi ve Sürtünme)
                    objData.vz = objData.vz - gravity
                    objData.vx = objData.vx * (1 - 0.015 * timeScale)
                    objData.vy = objData.vy * (1 - 0.015 * timeScale)
                    objData.vz = objData.vz * (1 - 0.010 * timeScale)
                    
                    local currentRadius = (CONFIG.OBJ_RADIUS / CONFIG.OBJ_SCALE) * objData.scale
                    local moveDist = math.sqrt(objData.vx^2 + objData.vy^2 + objData.vz^2)
                    
                    local dirX, dirY, dirZ = 0, 0, 0
                    if moveDist > 0 then
                        dirX, dirY, dirZ = objData.vx / moveDist, objData.vy / moveDist, objData.vz / moveDist
                    end
                    
                    local checkDist = (moveDist * timeScale) + currentRadius
                    local hit, hitX, hitY, hitZ, _, normalX, normalY, normalZ = processLineOfSight(
                        px, py, pz, px + dirX * checkDist, py + dirY * checkDist, pz + dirZ * checkDist,
                        true, true, false, true, true, false, false, false, objData.element
                    )
                    
                    if hit then
                        local distToHit = getDistanceBetweenPoints3D(px, py, pz, hitX, hitY, hitZ)
                        local safeMove = math.max(0, distToHit - currentRadius)
                        
                        px = px + dirX * safeMove
                        py = py + dirY * safeMove
                        pz = pz + dirZ * safeMove
                        
                        if safeMove <= 0 then
                            local pen = currentRadius - distToHit
                            px, py, pz = px + normalX * pen, py + normalY * pen, pz + normalZ * pen
                        end
                        
                        local dot = (objData.vx * normalX) + (objData.vy * normalY) + (objData.vz * normalZ)
                        
                        if dot < 0 then
                            -- UNREAL ENGINE TARZI SIVI PARÇALANMA SİSTEMİ (LIQUID SHATTER)
                            if objData.type == "water" and math.abs(dot) > 0.08 then
                                fxAddWaterSplash(hitX, hitY, hitZ)
                                
                                local dropCount = math.random(15, 20)
                                for d = 1, dropCount do
                                    local drop = createObject(CONFIG.TOP_ID, hitX, hitY, hitZ)
                                    setElementCollisionsEnabled(drop, false)
                                    
                                    local dropScale = objData.scale * (0.10 + math.random() * 0.15)
                                    setObjectScale(drop, dropScale)
                                    
                                    if waterShader then
                                        engineApplyShaderToWorldTexture(waterShader, "*", drop)
                                    end
                                    
                                    local outX = normalX + (math.random() - 0.5) * 2.0
                                    local outY = normalY + (math.random() - 0.5) * 2.0
                                    local outZ = normalZ + (math.random() - 0.5) * 2.0
                                    local len = math.sqrt(outX^2 + outY^2 + outZ^2)
                                    
                                    local burstSpeed = 0.2 + math.random() * 0.4
                                    
                                    table.insert(physicsObjects, {
                                        element = drop,
                                        type = "water_drop",
                                        vx = objData.vx * 0.4 + (outX / len) * burstSpeed,
                                        vy = objData.vy * 0.4 + (outY / len) * burstSpeed,
                                        vz = math.abs(objData.vz) * 0.4 + (outZ / len) * burstSpeed,
                                        rx = 0, ry = 0, rz = 0,
                                        scale = dropScale
                                    })
                                end
                                
                                destroyElement(objData.element)
                                table.remove(physicsObjects, i)
                                break 
                            end

                            -- Zemin Freni
                            if normalZ > 0.7 and math.abs(dot) < 0.05 then
                                objData.vx = objData.vx - dot * normalX
                                objData.vy = objData.vy - dot * normalY
                                objData.vz = 0
                                
                                local friction = 0.85
                                if objData.type == "box" then friction = 0.60 end
                                
                                objData.vx = objData.vx * friction
                                objData.vy = objData.vy * friction
                            else
                                -- Yansıma (Bounce)
                                local rx = objData.vx - 2 * dot * normalX
                                local ry = objData.vy - 2 * dot * normalY
                                local rz = objData.vz - 2 * dot * normalZ
                                
                                if objData.type == "water_drop" then
                                    objData.vx, objData.vy, objData.vz = rx * 0.5, ry * 0.5, rz * 0.3
                                elseif objData.type == "ball" then
                                    objData.vx, objData.vy, objData.vz = rx * 0.9, ry * 0.9, rz * 0.55
                                else
                                    objData.vx, objData.vy, objData.vz = rx * 0.8, ry * 0.8, rz * 0.15
                                end
                            end
                        end
                    else
                        px = px + objData.vx * timeScale
                        py = py + objData.vy * timeScale
                        pz = pz + objData.vz * timeScale
                    end
                    
                    -- OYUNCU İTME
                    local distToPlayer = getDistanceBetweenPoints3D(px, py, pz, ppx, ppy, ppz)
                    if distToPlayer < (currentRadius + 0.8) and playerSpeed > 0.02 then
                        local pushDist = math.sqrt((px - ppx)^2 + (py - ppy)^2)
                        if pushDist > 0 then
                            objData.vx = objData.vx + ((px - ppx) / pushDist) * playerSpeed * 0.6
                            objData.vy = objData.vy + ((py - ppy) / pushDist) * playerSpeed * 0.6
                        end
                    end
                    
                    if math.abs(objData.vx) < 0.001 then objData.vx = 0 end
                    if math.abs(objData.vy) < 0.001 then objData.vy = 0 end
                    
                    if objData.type == "ball" or objData.type == "water_drop" then
                        local speedXY = math.sqrt(objData.vx^2 + objData.vy^2)
                        if speedXY > 0.001 then
                            objData.rx = (objData.rx + objData.vy * (50 / objData.scale) * timeScale) % 360
                            objData.ry = (objData.ry - objData.vx * (50 / objData.scale) * timeScale) % 360
                        end
                    end
                    
                    local groundZ = getGroundPosition(px, py, pz + 0.5)
                    if groundZ then
                        local minZ = groundZ + currentRadius
                        if pz < minZ then
                            pz = minZ
                            if objData.vz < 0 and objData.vz > -0.05 then objData.vz = 0 end
                        end
                    end
                    
                    setElementPosition(objData.element, px, py, pz)
                    if objData.type == "ball" or objData.type == "water_drop" then
                        setElementRotation(objData.element, objData.rx, objData.ry, objData.rz)
                    end
                end
            end
        else
            table.remove(physicsObjects, i)
        end
    end
end)
