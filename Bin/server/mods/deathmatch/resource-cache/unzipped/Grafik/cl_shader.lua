-- SparroW MTA : https://sparrow-mta.blogspot.com
-- Facebook : https://www.facebook.com/sparrowgta/
-- İnstagram : https://www.instagram.com/sparrowmta/
-- Discord : https://discord.gg/DzgEcvy

textyre_doroga = {}
shader_doroga = {}

-- Отрисовка шейдеров 
function hdroad(state)
	if state == true then
		textyre_doroga[1] = dxCreateTexture("files/hdroad/vegasdirtyroad3_256.png", "dxt3")
		shader_doroga[1] = dxCreateShader("files/hdroad/shader.fx")
		dxSetShaderValue(shader_doroga[1], "gTexture", textyre_doroga[1])
		engineApplyShaderToWorldTexture(shader_doroga[1], "vegasdirtyroad3_256")
		
		textyre_doroga[2] = dxCreateTexture("files/hdroad/Tar_1line256HVblend2.png", "dxt3")
		shader_doroga[2] = dxCreateShader("files/hdroad/shader.fx")
		dxSetShaderValue(shader_doroga[2], "gTexture", textyre_doroga[2])
		engineApplyShaderToWorldTexture(shader_doroga[2], "vegasdirtyroad3_256")
		engineApplyShaderToWorldTexture(shader_doroga[2], "Tar_1line256HVblend2")
		engineApplyShaderToWorldTexture(shader_doroga[2], "Tar_1line256HVblenddrt")
		engineApplyShaderToWorldTexture(shader_doroga[2], "Tar_1line256HVblenddrtdot")
		engineApplyShaderToWorldTexture(shader_doroga[2], "Tar_1line256HVgtravel")
		engineApplyShaderToWorldTexture(shader_doroga[2], "Tar_1line256HVlightsand")
		engineApplyShaderToWorldTexture(shader_doroga[2], "Tar_lineslipway")
		engineApplyShaderToWorldTexture(shader_doroga[2], "Tar_venturasjoin")
		engineApplyShaderToWorldTexture(shader_doroga[2], "conc_slabgrey_256128")
		
		textyre_doroga[3] = dxCreateTexture("files/hdroad/snpedtest1BLND.png", "dxt3")
		shader_doroga[3] = dxCreateShader("files/hdroad/shader.fx")
		dxSetShaderValue(shader_doroga[3], "gTexture", textyre_doroga[3])
		engineApplyShaderToWorldTexture(shader_doroga[3], "ws_freeway3blend")
		engineApplyShaderToWorldTexture(shader_doroga[3], "snpedtest1BLND")
		engineApplyShaderToWorldTexture(shader_doroga[3], "vegastriproad1_256")
		
		textyre_doroga[4] = dxCreateTexture("files/hdroad/desert_1line256.png", "dxt3")
		shader_doroga[4] = dxCreateShader("files/hdroad/shader.fx")
		dxSetShaderValue(shader_doroga[4], "gTexture", textyre_doroga[4])
		engineApplyShaderToWorldTexture(shader_doroga[4], "desert_1line256")
		engineApplyShaderToWorldTexture(shader_doroga[4], "desert_1linetar")
		engineApplyShaderToWorldTexture(shader_doroga[4], "roaddgrassblnd")
		
		textyre_doroga[5] = dxCreateTexture("files/hdroad/crossing2_law.png", "dxt3")
		shader_doroga[5] = dxCreateShader("files/hdroad/shader.fx")
		dxSetShaderValue(shader_doroga[5], "gTexture", textyre_doroga[5])
		engineApplyShaderToWorldTexture(shader_doroga[5], "crossing2_law")
		engineApplyShaderToWorldTexture(shader_doroga[5], "lasunion994")
		engineApplyShaderToWorldTexture(shader_doroga[5], "motocross_256")
		
		textyre_doroga[6] = dxCreateTexture("files/hdroad/crossing_law.png", "dxt3")
		shader_doroga[6] = dxCreateShader("files/hdroad/shader.fx")
		dxSetShaderValue(shader_doroga[6], "gTexture", textyre_doroga[6])
		engineApplyShaderToWorldTexture(shader_doroga[6], "crossing_law")
		engineApplyShaderToWorldTexture(shader_doroga[6], "crossing_law2")
		engineApplyShaderToWorldTexture(shader_doroga[6], "crossing_law3")
		engineApplyShaderToWorldTexture(shader_doroga[6], "sf_junction5")
		engineApplyShaderToWorldTexture(shader_doroga[6], "crossing_law.bmp")
				
		textyre_doroga[7] = dxCreateTexture("files/hdroad/dt_road_stoplinea.png", "dxt3")
		shader_doroga[7] = dxCreateShader("files/hdroad/shader.fx")
		dxSetShaderValue(shader_doroga[7], "gTexture", textyre_doroga[7])
		engineApplyShaderToWorldTexture(shader_doroga[7], "dt_road_stoplinea")
		
		textyre_doroga[8] = dxCreateTexture("files/hdroad/Tar_freewyleft.png", "dxt3")
		shader_doroga[8] = dxCreateShader("files/hdroad/shader.fx")
		dxSetShaderValue(shader_doroga[8], "gTexture", textyre_doroga[8])
		engineApplyShaderToWorldTexture(shader_doroga[8], "Tar_freewyleft")
		
		textyre_doroga[9] = dxCreateTexture("files/hdroad/Tar_freewyright.png", "dxt3")
		shader_doroga[9] = dxCreateShader("files/hdroad/shader.fx")
		dxSetShaderValue(shader_doroga[9], "gTexture", textyre_doroga[9])
		engineApplyShaderToWorldTexture(shader_doroga[9], "Tar_freewyright")
		
		textyre_doroga[10] = dxCreateTexture("files/hdroad/Tar_1line256HV.png", "dxt3")
		shader_doroga[10] = dxCreateShader("files/hdroad/shader.fx")
		dxSetShaderValue(shader_doroga[10], "gTexture", textyre_doroga[10])
		engineApplyShaderToWorldTexture(shader_doroga[10], "Tar_1line256HV")
		engineApplyShaderToWorldTexture(shader_doroga[10], "Tar_1linefreewy")
		engineApplyShaderToWorldTexture(shader_doroga[10], "des_1line256")
		engineApplyShaderToWorldTexture(shader_doroga[10], "des_1lineend")
		engineApplyShaderToWorldTexture(shader_doroga[10], "des_1linetar")
		
		textyre_doroga[11] = dxCreateTexture("files/hdroad/sf_junction2.png", "dxt3")
		shader_doroga[11] = dxCreateShader("files/hdroad/shader.fx")
		dxSetShaderValue(shader_doroga[11], "gTexture", textyre_doroga[11])
		engineApplyShaderToWorldTexture(shader_doroga[11], "sf_junction2")
		
		textyre_doroga[12] = dxCreateTexture("files/hdroad/vegastriproad1_256.png", "dxt3")
		shader_doroga[12] = dxCreateShader("files/hdroad/shader.fx")
		dxSetShaderValue(shader_doroga[12], "gTexture", textyre_doroga[12])
		engineApplyShaderToWorldTexture(shader_doroga[12], "vegastriproad1_256")
		engineApplyShaderToWorldTexture(shader_doroga[12], "ws_freeway3")
		engineApplyShaderToWorldTexture(shader_doroga[12], "cuntroad01_law")
		engineApplyShaderToWorldTexture(shader_doroga[12], "roadnew4blend_256")
		engineApplyShaderToWorldTexture(shader_doroga[12], "sf_road5")
		engineApplyShaderToWorldTexture(shader_doroga[12], "sl_roadbutt1")
		engineApplyShaderToWorldTexture(shader_doroga[12], "snpedtest1")
		
		textyre_doroga[13] = dxCreateTexture("files/hdroad/vegastriproad1_256.png", "dxt3")
		shader_doroga[13] = dxCreateShader("files/hdroad/shader.fx")
		dxSetShaderValue(shader_doroga[13], "gTexture", textyre_doroga[13])
		engineApplyShaderToWorldTexture(shader_doroga[13], "vegastriproad1_256")
		engineApplyShaderToWorldTexture(shader_doroga[13], "ws_freeway3")
		engineApplyShaderToWorldTexture(shader_doroga[13], "cuntroad01_law")
		engineApplyShaderToWorldTexture(shader_doroga[13], "roadnew4blend_256")
		engineApplyShaderToWorldTexture(shader_doroga[13], "sf_road5")
		engineApplyShaderToWorldTexture(shader_doroga[13], "sl_roadbutt1")
		engineApplyShaderToWorldTexture(shader_doroga[13], "snpedtest1")
		
		textyre_doroga[14] = dxCreateTexture("files/hdroad/sl_freew2road1.png", "dxt3")
		shader_doroga[14] = dxCreateShader("files/hdroad/shader.fx")
		dxSetShaderValue(shader_doroga[14], "gTexture", textyre_doroga[14])
		engineApplyShaderToWorldTexture(shader_doroga[14], "sl_freew2road1")
		engineApplyShaderToWorldTexture(shader_doroga[14], "snpedtest1blend")
		engineApplyShaderToWorldTexture(shader_doroga[14], "ws_carpark3")
		
		textyre_doroga[15] = dxCreateTexture("files/hdroad/cos_hiwaymid_256.png", "dxt3")
		shader_doroga[15] = dxCreateShader("files/hdroad/shader.fx")
		dxSetShaderValue(shader_doroga[15], "gTexture", textyre_doroga[15])
		engineApplyShaderToWorldTexture(shader_doroga[15], "cos_hiwaymid_256")
		engineApplyShaderToWorldTexture(shader_doroga[15], "sf_road5")
		
		textyre_doroga[16] = dxCreateTexture("files/hdroad/hiwayend_256.png", "dxt3")
		shader_doroga[16] = dxCreateShader("files/hdroad/shader.fx")
		dxSetShaderValue(shader_doroga[16], "gTexture", textyre_doroga[16])
		engineApplyShaderToWorldTexture(shader_doroga[16], "hiwayend_256")
		engineApplyShaderToWorldTexture(shader_doroga[16], "hiwaymidlle_256")
		engineApplyShaderToWorldTexture(shader_doroga[16], "vegasroad2_256")
		
		textyre_doroga[17] = dxCreateTexture("files/hdroad/roadnew4_256.png", "dxt3")
		shader_doroga[17] = dxCreateShader("files/hdroad/shader.fx")
		dxSetShaderValue(shader_doroga[17], "gTexture", textyre_doroga[17])
		engineApplyShaderToWorldTexture(shader_doroga[17], "roadnew4_256")
		engineApplyShaderToWorldTexture(shader_doroga[17], "roadnew4_512")
		engineApplyShaderToWorldTexture(shader_doroga[17], "vegasroad1_256")
		engineApplyShaderToWorldTexture(shader_doroga[17], "dt_road")
		engineApplyShaderToWorldTexture(shader_doroga[17], "vgsN_road2sand01")
		engineApplyShaderToWorldTexture(shader_doroga[17], "hiwayoutside_256")
		engineApplyShaderToWorldTexture(shader_doroga[17], "vegasdirtyroad1_256")
		engineApplyShaderToWorldTexture(shader_doroga[17], "vegasdirtyroad2_256")
		engineApplyShaderToWorldTexture(shader_doroga[17], "vegasroad3_256")
		
		textyre_doroga[18] = dxCreateTexture("files/hdroad/sf_junction1.png", "dxt3")
		shader_doroga[18] = dxCreateShader("files/hdroad/shader.fx")
		dxSetShaderValue(shader_doroga[18], "gTexture", textyre_doroga[18])
		engineApplyShaderToWorldTexture(shader_doroga[18], "sf_junction1")
		engineApplyShaderToWorldTexture(shader_doroga[18], "sf_junction3")
		
		textyre_doroga[19] = dxCreateTexture("files/hdroad/des_oldrunway.png", "dxt3")
		shader_doroga[19] = dxCreateShader("files/hdroad/shader.fx")
		dxSetShaderValue(shader_doroga[19], "gTexture", textyre_doroga[19])
		engineApplyShaderToWorldTexture(shader_doroga[19], "des_oldrunway")
		engineApplyShaderToWorldTexture(shader_doroga[19], "des_panelconc")
		engineApplyShaderToWorldTexture(shader_doroga[19], "plaintarmac1")
	else
		for k, v in pairs(textyre_doroga) do
			if isElement(textyre_doroga[k]) then 
				destroyElement(textyre_doroga[k])
			end
		end
		for k, v in pairs(shader_doroga) do
			if isElement(shader_doroga[k]) then 
				engineRemoveShaderFromWorldTexture(shader_doroga[k], "*") 
				destroyElement(shader_doroga[k]) 
			end
		end
	end
end

textyre_water = {}
shader_water = {}

function hdwater(state)
	if state == true then
		myShader, tec = dxCreateShader ( "files/hdwater/water.fx" )
		textureVol = dxCreateTexture ( "files/hdwater/smallnoise3d.dds" );
		textureCube = dxCreateTexture ( "files/hdwater/cube_env256.dds" );
		dxSetShaderValue ( myShader, "microflakeNMapVol_Tex", textureVol );
		dxSetShaderValue ( myShader, "showroomMapCube_Tex", textureCube );
		engineApplyShaderToWorldTexture ( myShader, "waterclear256" )
		setTimer(	function()
			if isElement(myShader) then
				local r,g,b,a = getWaterColor()
				dxSetShaderValue ( myShader, "gWaterColor", r/255, g/255, b/255, a/255 );
			end
		end,100,0 )
	else
		if isElement(myShader) then 
			destroyElement(myShader)
		elseif isElement(textureVol) then
			destroyElement(myShader)
		elseif isElement(textureCube) then 
			destroyElement(myShader)
		end
	end
end