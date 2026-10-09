
local sphereObjScale=100 -- set object scale
local sphereShadScale={1,1,0.9} -- object scale in VS

local sphereTexScale={1,1,1} -- scale the texture in PS
-- color variables below
local ColorAdd=-0.15; -- 0 to -0.4 -- standard colors are too bright
local ColorPow=2; -- 1 to 2 -- contrast
local shadCloudsTexDisabled=false
local modelID=15057  -- that's probably the best model to replace ... or not
setWeather(10) -- the chosen weather (you might delete that but choose a proper one)
setCloudsEnabled(false)
local  getLastTick,getLastTock = 0,0

textyre_aero = {}
shader_aero = {}

function hdAero(state)
    if state == true then
      shader_aero[1] = dxCreateShader ( "files/hdaero/shaders/shader_skydome.fx" )
      shader_aero[2] = dxCreateShader ( "files/hdaero/shaders/shader_null.fx" )


 setCloudsEnabled(false)
 textyre_aero[1] = dxCreateTexture("files/hdaero/textures/skydome.jpg")
 dxSetShaderValue ( shader_aero[1], "gTEX", textyre_aero[1])
 dxSetShaderValue ( shader_aero[1], "gAlpha", 1 )
 dxSetShaderValue ( shader_aero[1], "gObjScale", sphereShadScale )
 dxSetShaderValue ( shader_aero[1], "gTexScale",sphereTexScale )
 dxSetShaderValue ( shader_aero[1], "gColorAdd",ColorAdd)
 dxSetShaderValue ( shader_aero[1], "gColorPow",ColorPow)

 -- apply to texture
 engineApplyShaderToWorldTexture ( shader_aero[1], "skybox_tex" ) 
 if shadCloudsTexDisabled then engineApplyShaderToWorldTexture ( shader_aero[2], "cloudmasked*" ) end
 txd_skybox = engineLoadTXD('files/hdaero/models/skybox_model.txd')
 engineImportTXD(txd_skybox, modelID)
 dff_skybox = engineLoadDFF('files/hdaero/models/skybox_model.dff', modelID)
 engineReplaceModel(dff_skybox, modelID,true)  
 
 local cam_x,cam_y,cam_z = getElementPosition(getLocalPlayer())
 skyBoxBoxa = createObject ( modelID, cam_x, cam_y, cam_z, 0, 0, 0, true )
 setObjectScale(skyBoxBoxa,sphereObjScale) 
 addEventHandler ( "onClientHUDRender", getRootElement (), renderSphere ) -- sky object
 addEventHandler ( "onClientHUDRender", getRootElement (), renderTime ) -- time 
 applyWeatherInfluence()

    else
      for k, v in pairs(textyre_aero) do
        if isElement(textyre_aero[k]) then 
          destroyElement(textyre_aero[k])
        end
      end
      for k, v in pairs(shader_aero) do
        if isElement(shader_aero[k]) then 
          engineRemoveShaderFromWorldTexture(shader_aero[k], "*") 
          destroyElement(shader_aero[k]) 
        end
      end
    end
end

lastWeather=0
function renderSphere()
 -- Updates the position of the object
 if getTickCount ( ) - getLastTock < 2  then return end
 local cam_x, cam_y, cam_z, lx, ly, lz = getCameraMatrix()
 if cam_z<=200 then setElementPosition ( skyBoxBoxa, cam_x, cam_y, 80,false ) 
 else setElementPosition ( skyBoxBoxa, cam_x, cam_y, 80+cam_z-200,false )  end
 if getWeather()~=lastWeather then applyWeatherInfluence() end
 lastWeather=getWeather()
 getLastTock = getTickCount ()
end

function renderTime()

 local hour, minute = getTime ( )
 if getTickCount ( ) - getLastTick < 100  then return end
 if not shader_aero[1] then return end
 if hour >= 20 then
  local dusk_aspect = ((hour-20)*60+minute)/240
  dusk_aspect = 1-dusk_aspect
  dxSetShaderValue ( shader_aero[1], "gAlpha", dusk_aspect)
 end
  
 if hour <= 2 then
  dxSetShaderValue ( shader_aero[1], "gAlpha", 0)
 end
  
 if hour > 2 and hour <= 6 then
  local dawn_aspect = ((hour-3)*60+minute)/180
  dawn_aspect = dawn_aspect
  dxSetShaderValue ( shader_aero[1], "gAlpha", dawn_aspect)
 end
  
 if hour > 6 and hour < 20 then
  dxSetShaderValue ( shader_aero[1], "gAlpha", 1)
 end

 getLastTick = getTickCount ()
end


function applyWeatherInfluence()
  setSunSize (0)
  setSunColor(0, 0, 0, 0, 0, 0)
end
