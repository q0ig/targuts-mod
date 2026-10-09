-- SparroW MTA : https://sparrow-mta.blogspot.com
-- Facebook : https://www.facebook.com/sparrowgta/
-- İnstagram : https://www.instagram.com/sparrowmta/
-- Discord : https://discord.gg/DzgEcvy

local scx, scy = guiGetScreenSize()
local px, py = scx/1366, scy/768

GUI = {
	openWindow = false,
	elem = {},
	scroll = 0,
	hdroad = false,
	hdwater = false,
	hdaero = false,
	hdreflect = false
}
white = tocolor(255,255,255)

function calculateGUI()
	GUI.elem.background = {				
		left	= scx/2-320/2*px, 	
		top		= scy/2-470/2*py,								
		width	= 320*px,
		height	= 470*py
	}
	GUI.elem.namebg = {
		left	= GUI.elem.background.left, 	
		top		= GUI.elem.background.top,								
		width	= GUI.elem.background.width,
		height	= 50*py
	}
	GUI.elem.bg_line1 = {
		left	= GUI.elem.background.left+10*px, 	
		top		= GUI.elem.namebg.top+GUI.elem.namebg.height,								
		width	= GUI.elem.background.width-20*px,
		height	= 2*py
	}

	GUI.elem.hdroadText = {
		left	= GUI.elem.background.left+20*px, 	
		top		= GUI.elem.bg_line1.top+GUI.elem.bg_line1.height+20*py,								
		width	= GUI.elem.background.width/2-20*px,
		height	= 25*py,
		scale = 1.5
	}
	GUI.elem.hdroad = {
		left	= GUI.elem.hdroadText.left+GUI.elem.hdroadText.width+40, 	
		top		= GUI.elem.hdroadText.top,								
		width	= GUI.elem.background.width/5,
		height	= 25*py,
		scale = 1.5
	}
	GUI.elem.hdroadOff = {
		left	= GUI.elem.hdroad.left, 	
		top		= GUI.elem.hdroad.top,								
		width	= 25*px,
		height	= 25*py,
		scale = 1.5
	}
	GUI.elem.hdroadOn = {
		left	= GUI.elem.hdroad.left+GUI.elem.hdroad.width-25*px, 	
		top		= GUI.elem.hdroad.top,								
		width	= GUI.elem.hdroadOff.width,
		height	= GUI.elem.hdroadOff.height,
		scale = 1.5
	}




	GUI.elem.hdwaterText = {
		left	= GUI.elem.hdroadText.left, 	
		top		= GUI.elem.hdroadText.top+GUI.elem.hdroadText.height+5*py,								
		width	= GUI.elem.hdroadText.width,
		height	= GUI.elem.hdroadText.height,
		scale = 1.5
	}
	GUI.elem.hdwater = {
		left	= GUI.elem.hdroad.left, 	
		top		= GUI.elem.hdwaterText.top,								
		width	= GUI.elem.hdroad.width,
		height	= 25*py,
		scale = 1.5
	}
	GUI.elem.hdwaterOff = {
		left	= GUI.elem.hdwater.left, 	
		top		= GUI.elem.hdwater.top,								
		width	= GUI.elem.hdroadOff.width,
		height	= GUI.elem.hdroadOff.height,
		scale = 1.5
	}
	GUI.elem.hdwaterOn = {
		left	= GUI.elem.hdwater.left+GUI.elem.hdwater.width-25*px, 	
		top		= GUI.elem.hdwater.top,								
		width	= GUI.elem.hdroadOff.width,
		height	= GUI.elem.hdroadOff.height,
		scale = 1.5
	}

	GUI.elem.hdaeroText = {
		left	= GUI.elem.hdroadText.left, 	
		top		= GUI.elem.hdwaterText.top+GUI.elem.hdwaterText.height+5*py,								
		width	= GUI.elem.hdroadText.width,
		height	= GUI.elem.hdroadText.height,
		scale = 1.5
	}
	GUI.elem.hdaero = {
		left	= GUI.elem.hdroad.left, 	
		top		= GUI.elem.hdaeroText.top,								
		width	= GUI.elem.hdroad.width,
		height	= 25*py,
		scale = 1.5
	}
	GUI.elem.hdaeroOff = {
		left	= GUI.elem.hdaero.left, 	
		top		= GUI.elem.hdaero.top,								
		width	= GUI.elem.hdroadOff.width,
		height	= GUI.elem.hdroadOff.height,
		scale = 1.5
	}
	GUI.elem.hdaeroOn = {
		left	= GUI.elem.hdaero.left+GUI.elem.hdaero.width-25*px, 	
		top		= GUI.elem.hdaero.top,								
		width	= GUI.elem.hdroadOff.width,
		height	= GUI.elem.hdroadOff.height,
		scale = 1.5
	}

	GUI.elem.hdreflectText = {
		left	= GUI.elem.hdroadText.left, 	
		top		= GUI.elem.hdaeroText.top+GUI.elem.hdaeroText.height+5*py,								
		width	= GUI.elem.hdroadText.width,
		height	= GUI.elem.hdroadText.height,
		scale = 1.5
	}
	GUI.elem.hdreflect = {
		left	= GUI.elem.hdroad.left, 	
		top		= GUI.elem.hdreflectText.top,								
		width	= GUI.elem.hdroad.width,
		height	= 25*py,
		scale = 1.5
	}
	GUI.elem.hdreflectOff = {
		left	= GUI.elem.hdreflect.left, 	
		top		= GUI.elem.hdreflect.top,								
		width	= GUI.elem.hdroadOff.width,
		height	= GUI.elem.hdroadOff.height,
		scale = 1.5
	}
	GUI.elem.hdreflectOn = {
		left	= GUI.elem.hdreflect.left+GUI.elem.hdreflect.width-25*px, 	
		top		= GUI.elem.hdreflect.top,								
		width	= GUI.elem.hdroadOff.width,
		height	= GUI.elem.hdroadOff.height,
		scale = 1.5
	}


	GUI.elem.bg_line2 = {
		left	= GUI.elem.background.left+10*px, 	
		top		= GUI.elem.hdreflectText.top+GUI.elem.hdreflectText.height+20*py,								
		width	= GUI.elem.background.width-20*px,
		height	= 2*py
	}

	GUI.elem.portrayalText = {
		left	= GUI.elem.background.left, 	
		top		= GUI.elem.bg_line2.top+GUI.elem.bg_line2.height+10*py,								
		width	= GUI.elem.background.width,
		height	= GUI.elem.hdroadText.height,
		scale = 1.5
	}


	GUI.elem.autotuningText = {
		left	= GUI.elem.background.left, 	
		top		= GUI.elem.portrayalText.top+GUI.elem.portrayalText.height+50*py,								
		width	= GUI.elem.background.width,
		height	= 20*py,
		scale = 1.5
	}

	GUI.elem.autotuningWeak = {
		left	= GUI.elem.background.left+25, 	
		top		= GUI.elem.autotuningText.top+GUI.elem.autotuningText.height+15*py,								
		width	= GUI.elem.background.width-50,
		height	= 20*py,
		scale = 1.0
	}
	GUI.elem.autotuningaverage = {
		left	= GUI.elem.autotuningWeak.left, 	
		top		= GUI.elem.autotuningWeak.top+GUI.elem.autotuningWeak.height+15*py,								
		width	= GUI.elem.autotuningWeak.width,
		height	= GUI.elem.autotuningWeak.height,
		scale = GUI.elem.autotuningWeak.scale
	}
	GUI.elem.autotuningGood = {
		left	= GUI.elem.autotuningWeak.left, 	
		top		= GUI.elem.autotuningaverage.top+GUI.elem.autotuningaverage.height+15*py,								
		width	= GUI.elem.autotuningWeak.width,
		height	= GUI.elem.autotuningWeak.height,
		scale = GUI.elem.autotuningWeak.scale
	}
	GUI.elem.bg_line3 = {
		left	= GUI.elem.background.left+10*px, 	
		top		= GUI.elem.autotuningGood.top+GUI.elem.autotuningGood.height+20*py,								
		width	= GUI.elem.background.width-20*px,
		height	= 2*py
	}
	
	GUI.elem.infoExit = {
		left	= GUI.elem.background.left, 	
		top		= GUI.elem.bg_line3.top+GUI.elem.bg_line3.height+5*py,								
		width	= GUI.elem.background.width,
		height	= 20*py,
		scale = 1.00
	}
end
calculateGUI()

addEventHandler("onClientRender", root, function()
	if (GUI.openWindow) then
		drawElem(GUI.elem.background,"files/bg.png",tocolor(255,255,255))
		drawTextInElem(GUI.elem.namebg,"Grafik Paneli",2,"default-bold","center","center")
		drawRen(GUI.elem.bg_line1,tocolor(86,95,114,255))
		drawTextInElem(GUI.elem.hdroadText,"HD Yol",GUI.elem.hdroadText.scale,"default-bold","left","center")
		if GUI.hdroad then
			drawElem(GUI.elem.hdroad,"files/key2.png",tocolor(96,133,93))
			drawElem(GUI.elem.hdroadOn,"files/select.png",tocolor(255,255,255))
		else
			drawElem(GUI.elem.hdroad,"files/key2.png",tocolor(216,128,141))
			drawElem(GUI.elem.hdroadOff,"files/select.png",tocolor(255,255,255))
		end

		drawTextInElem(GUI.elem.hdwaterText,"HD Deniz",GUI.elem.hdwaterText.scale,"default-bold","left","center")
		if GUI.hdwater then
			drawElem(GUI.elem.hdwater,"files/key2.png",tocolor(96,133,93))
			drawElem(GUI.elem.hdwaterOn,"files/select.png",tocolor(255,255,255))
		else
			drawElem(GUI.elem.hdwater,"files/key2.png",tocolor(216,128,141))
			drawElem(GUI.elem.hdwaterOff,"files/select.png",tocolor(255,255,255))
		end

		drawTextInElem(GUI.elem.hdaeroText,"HD Gökyüzü",GUI.elem.hdaeroText.scale,"default-bold","left","center")
		if GUI.hdaero then
			drawElem(GUI.elem.hdaero,"files/key2.png",tocolor(96,133,93))
			drawElem(GUI.elem.hdaeroOn,"files/select.png",tocolor(255,255,255))
		else
			drawElem(GUI.elem.hdaero,"files/key2.png",tocolor(216,128,141))
			drawElem(GUI.elem.hdaeroOff,"files/select.png",tocolor(255,255,255))
		end

		drawTextInElem(GUI.elem.hdreflectText,"HD Araç",GUI.elem.hdreflectText.scale,"default-bold","left","center")
		if GUI.hdreflect then
			drawElem(GUI.elem.hdreflect,"files/key2.png",tocolor(96,133,93))
			drawElem(GUI.elem.hdreflectOn,"files/select.png",tocolor(255,255,255))
		else
			drawElem(GUI.elem.hdreflect,"files/key2.png",tocolor(216,128,141))
			drawElem(GUI.elem.hdreflectOff,"files/select.png",tocolor(255,255,255))
		end

		drawRen(GUI.elem.bg_line2,tocolor(86,95,114,255))

		drawTextInElem(GUI.elem.portrayalText,"Görüş Mesafesi",GUI.elem.portrayalText.scale,"default-bold","center","center")
		
		GUI.elem.portrayal = {
			left	= GUI.elem.background.left+25, 	
			top		= GUI.elem.portrayalText.top+GUI.elem.portrayalText.height+15*py,								
			width	= GUI.elem.background.width-50*px,
			height	= 20*py,
			scale = 1.5
		}

		GUI.elem.portrayalScrool = {
			left	= GUI.elem.portrayal.left+GUI.scroll, 	
			top		= GUI.elem.portrayal.top,								
			width	= 20*px,
			height	= 20*py,
			scale = 1.5
		}
		GUI.elem.portrayalRed = {
			left	= GUI.elem.portrayal.left+GUI.scroll+10*px, 	
			top		= GUI.elem.portrayal.top,								
			width	= GUI.elem.background.width-60*px-GUI.scroll,
			height	= 20*py,
			scale = 1.5
		}
		drawElem(GUI.elem.portrayal,"files/scroll.png",tocolor(96,133,93))
		drawElem(GUI.elem.portrayalRed,"files/scroll.png",tocolor(216,128,141))
		drawElem(GUI.elem.portrayalScrool,"files/select.png",tocolor(255,255,255))
		if isCursorInElement(GUI.elem.portrayalScrool) then
			if getKeyState("mouse1") then
				local cx,cy = getCursorPosition()
				local mx,my = cx*scx,cy*scy
				GUI.scroll = mx-GUI.elem.portrayal.left-14*px 
				if GUI.scroll >= GUI.elem.portrayal.width-20*px then GUI.scroll = GUI.elem.portrayal.width-20*px
				elseif GUI.scroll <= 0 then GUI.scroll = 0 end
				setFarClipDistance(1000+GUI.scroll*16)
				setElementData(localPlayer,"SettingClipDistance",GUI.scroll)
				-- 250 мах

				--print(scroll)
			end	

		end	


		drawTextInElem(GUI.elem.autotuningText,"Gökyüzü Ayarları",GUI.elem.autotuningText.scale,"default-bold","center","center")
		
		drawElem(GUI.elem.autotuningWeak,"files/key.png",tocolor(255,0,0))
		drawTextInElem(GUI.elem.autotuningWeak,"Düşük Sistemli Bilgisayar",GUI.elem.autotuningWeak.scale,"default-bold","center","center")

		drawElem(GUI.elem.autotuningaverage,"files/key.png",tocolor(0,85,255))
		drawTextInElem(GUI.elem.autotuningaverage,"Orta Sistemli Bilgisayar",GUI.elem.autotuningaverage.scale,"default-bold","center","center")

		drawElem(GUI.elem.autotuningGood,"files/key.png",tocolor(43,255,0))
		drawTextInElem(GUI.elem.autotuningGood,"Yüksek Sistemli Bilgisayar",GUI.elem.autotuningGood.scale,"default-bold","center","center")
		
		drawRen(GUI.elem.bg_line3,tocolor(86,95,114,255))

		drawTextInElem(GUI.elem.infoExit,"www.sparrow-mta.blogspot.com",GUI.elem.infoExit.scale,"default","center","center",false,tocolor(86,95,114))



	end
end)
addEventHandler("onClientClick", root, function(but, state, x, y)
	if (GUI.openWindow) and (but == "left") and (state == "up") then		
		if isCursorInElement(GUI.elem.hdroad) then
			if not GUI.hdroad then 
				GUI.hdroad = true 
				hdroad(true)
				setElementData(localPlayer,"Settinghdroad",true)
			else 
				GUI.hdroad = false
				hdroad(false) 
				setElementData(localPlayer,"Settinghdroad",false)
			end
		elseif isCursorInElement(GUI.elem.hdwater) then
			if GUI.hdwater then 
				GUI.hdwater = false 
				hdwater(false)
				setElementData(localPlayer,"Settinghdwater",true)
			else 
				GUI.hdwater = true 
				hdwater(true)
				setElementData(localPlayer,"Settinghdwater",false)
			end
		elseif isCursorInElement(GUI.elem.hdaero) then
			if GUI.hdaero then 
				GUI.hdaero = false 
				hdAero(false)
				setElementData(localPlayer,"Settinghdaero",true)
			else 
				GUI.hdaero = true
				hdAero(true)
				setElementData(localPlayer,"Settinghdaero",false)
			end
		elseif isCursorInElement(GUI.elem.hdreflect) then
			if GUI.hdreflect then 
				GUI.hdreflect = false
				hdReflect(false)
				setElementData(localPlayer,"Settinghdreflect",true)
			else 
				GUI.hdreflect = true 
				hdReflect(true)
				setElementData(localPlayer,"Settinghdreflect",false)
			end
		elseif isCursorInElement(GUI.elem.autotuningWeak) then
			GUI.scroll = 0 -- 1к дистанция
			setFarClipDistance(1000+GUI.scroll*16)
			
			updateSeting("Weak")
		elseif isCursorInElement(GUI.elem.autotuningaverage) then
			GUI.scroll = 125 -- 2.5к дистанция
			setFarClipDistance(1000+GUI.scroll*16)
			updateSeting("Average")
		elseif isCursorInElement(GUI.elem.autotuningGood) then
			GUI.scroll = 250 -- 5к дистанция
			setFarClipDistance(1000+GUI.scroll*16)
			updateSeting("Good")
		
		end
	end
end)

function updateSeting(state)
	if state == "Weak" then
		if not GUI.hdroad then
			hdroad(true)
			GUI.hdroad = true
		end
		if GUI.hdwater then
			hdwater(false)
			GUI.hdwater = false
		end
		if GUI.hdaero then
			hdAero(false)
			GUI.hdaero = false
		end
		if GUI.hdreflect then
			hdReflect(false)
			GUI.hdreflect = false
		end
		GUI.scroll = 0 -- 1к дистанция
		setFarClipDistance(1000+GUI.scroll*16)

		setElementData(localPlayer,"Settinghdroad",true)
		setElementData(localPlayer,"Settinghdwater",false)
		setElementData(localPlayer,"Settinghdaero",false)
		setElementData(localPlayer,"Settinghdreflect",false)
		setElementData(localPlayer,"SettingClipDistance",GUI.scroll)

	elseif state == "Average" then
		if not GUI.hdroad then
			hdroad(true)
			GUI.hdroad = true
		end
		if not GUI.hdwater then
			hdwater(true)
			GUI.hdwater = true
		end
		if GUI.hdaero then
			hdAero(false)
			GUI.hdaero = false
		end
		if GUI.hdreflect then
			hdReflect(false)
			GUI.hdreflect = false
		end

		GUI.scroll = 125 -- 2.5к дистанция
		setFarClipDistance(1000+GUI.scroll*16)


		setElementData(localPlayer,"Settinghdroad",true)
		setElementData(localPlayer,"Settinghdwater",true)
		setElementData(localPlayer,"Settinghdaero",false)
		setElementData(localPlayer,"Settinghdreflect",false)
		setElementData(localPlayer,"SettingClipDistance",GUI.scroll)
	elseif state == "Good" then
		if not GUI.hdroad then
			hdroad(true)
			GUI.hdroad = true
		end
		if not GUI.hdwater then
			hdwater(true)
			GUI.hdwater = true
		end
		if not GUI.hdaero then
			hdAero(true)
			GUI.hdaero = true
		end
		if not GUI.hdreflect then
			hdReflect(true)
			GUI.hdreflect = true
		end

		GUI.scroll = 250 -- 5к дистанция
		setFarClipDistance(1000+GUI.scroll*16)
		setElementData(localPlayer,"Settinghdroad",true)
		setElementData(localPlayer,"Settinghdwater",true)
		setElementData(localPlayer,"Settinghdaero",true)
		setElementData(localPlayer,"Settinghdreflect",true)
		setElementData(localPlayer,"SettingClipDistance",GUI.scroll)
	end
end
addEvent("updateSeting",true)
addEventHandler("updateSeting",root,updateSeting)

function setSettingGraphis(data)
	if data.road then
		hdroad(true)
		GUI.hdroad = true
	end
	if data.water then
		hdwater(true)
		GUI.hdwater = true
	end
	if data.aero then
		hdAero(true)
		GUI.hdaero = true
	end
	if data.reflect then
		hdReflect(true)
		GUI.hdreflect = true
	end
	if data.dist then
		GUI.scroll = data.dist
		setFarClipDistance(1000+GUI.scroll*16)
	end
end
addEvent("setSettingGraphis",true)
addEventHandler("setSettingGraphis",root,setSettingGraphis)


function openShaderSetting()
	if not GUI.openWindow then 
		GUI.openWindow = true 
		showCursor(true)
	else 
		GUI.openWindow = false
		showCursor(false)
	end
end
bindKey("5","down",openShaderSetting) -- tuş -- bind - panel açma tuşu


-- ==========     Отрисовка отдельных элементов     ==========

function drawRen(elem,color)
	dxDrawRectangle(elem.left, elem.top, elem.width, elem.height,color or white,false)
end
function drawElem(elem, path,color)
	dxDrawImage(elem.left, elem.top, elem.width, elem.height, path, 0, 0, 0, color or white, false)
end


function drawTextInElem(elem, text, scale, font, alignX, alignY, wordBreak, color)
	dxDrawText(text, elem.left, elem.top, elem.left+elem.width, elem.top+elem.height, color or white, scale, font, alignX, alignY, true, wordBreak, false, true, false)
end

function isCursorInElement(elem)
	local x, y = getCursorPosition()
	x = (x or 0) * scx
	y = (y or 0) * scy
	return (elem.left < x) and (x < elem.left+elem.width) and (elem.top < y) and (y < elem.top+elem.height)
end

-- SparroW MTA : https://sparrow-mta.blogspot.com
-- Facebook : https://www.facebook.com/sparrowgta/
-- İnstagram : https://www.instagram.com/sparrowmta/
-- Discord : https://discord.gg/DzgEcvy