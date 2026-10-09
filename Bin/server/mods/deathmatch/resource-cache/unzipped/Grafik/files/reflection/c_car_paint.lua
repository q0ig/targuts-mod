--
-- c_car_paint.lua
local myShader99 = dxCreateShader ( "files/reflection/car_paint.fx" )
addEventHandler( "onClientResourceStart", resourceRoot,
    function()

	if getVersion ().sortable < "1.1.0" then
		outputChatBox( "Resource is not compatible with this client." )
		return
	end

	if not myShader99 then
		outputChatBox( "Could not create shader. Please use debugscript 3" )
	else
    	dxSetShaderValue ( myShader99, "sRandomTexture", dxCreateTexture ( "files/reflection/smallnoise3d.dds" ) );
		dxSetShaderValue ( myShader99, "sReflectionTexture", dxCreateTexture ( "files/reflection/cube_env256.dds" ) );
	end
end)		
		
function hdReflect(data)		
	if data then
		engineApplyShaderToWorldTexture ( myShader99, "vehiclegrunge256" )
		engineApplyShaderToWorldTexture ( myShader99, "?emap*" )
	else
		engineRemoveShaderFromWorldTexture  ( myShader99, "vehiclegrunge256" )
		engineRemoveShaderFromWorldTexture ( myShader99, "?emap*" )
	end
end


	    

			
			
			

			

		


