-- SparroW MTA : https://sparrow-mta.blogspot.com
-- Facebook : https://www.facebook.com/sparrowgta/
-- İnstagram : https://www.instagram.com/sparrowmta/
-- Discord : https://discord.gg/DzgEcvy

addEventHandler("onPlayerLogin",root,function (_,acc)
	data = getAccountData(acc,"settingGrafickPatch") and fromJSON(getAccountData(acc,"settingGrafickPatch")) or {}
	if data then
		triggerClientEvent(source,"setSettingGraphis",source,data)
	else
		triggerClientEvent(source,"updateSeting",source,"Good")
	end
end)


addEventHandler("onPlayerQuit",root,function ()
	acc = getPlayerAccount(source)

	road = getElementData(source,"Settinghdroad") or true
	water = getElementData(source,"Settinghdwater") or true
	aero = getElementData(source,"Settinghdaero") or true
	reflect = getElementData(source,"Settinghdreflect") or true
	dist = getElementData(source,"SettingClipDistance") or 250

	local playerProps = getAccountData(acc, "settingGrafickPatch")
	playerProps = (playerProps) and fromJSON(playerProps) or {}
	playerProps.road = road
	playerProps.water = water
	playerProps.aero = aero
	playerProps.reflect = reflect
	playerProps.dist = dist

	setAccountData(acc, "settingGrafickPatch", toJSON(playerProps, true))

end)
