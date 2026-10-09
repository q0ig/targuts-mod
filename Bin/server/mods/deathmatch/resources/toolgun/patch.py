import zipfile
import xml.etree.ElementTree as ET
import os

zip_path = r'c:\Users\dogu\Desktop\server\mods\deathmatch\resources\[editor]\editor_gui.zip'
shared_lua_path = r'c:\Users\dogu\Desktop\server\mods\deathmatch\resources\toolgun\shared.lua'
client_lua_path = r'c:\Users\dogu\Desktop\server\mods\deathmatch\resources\toolgun\client.lua'

objects_items = []
try:
    with zipfile.ZipFile(zip_path, 'r') as z:
        xml_data = z.read('client/browser/objects.xml')
        root = ET.fromstring(xml_data)
        
        for group in root.findall('group'):
            for obj in group.findall('object'):
                id_str = obj.get('model') or obj.get('id')
                name_str = obj.get('name')
                if id_str and name_str:
                    name_str = name_str.replace('"', '').replace('\\', '')
                    objects_items.append(f'        {{ id = {id_str}, name = "{name_str}" }},')
except Exception as e:
    print('Failed to read objects.xml:', e)

if not objects_items:
    print('No objects found, falling back to default')
    objects_items = ['        { id = 1337, name = "Trash Can" },', '        { id = 2969, name = "Red Container" }']

shared_lua_content = f"""-- ====================================================================
-- GARRYS MOD TOOL GUN: SHARED CONFIGURATION & CATALOG
-- ====================================================================

TOOLGUN_CONFIG = {{
    weaponId = 23, -- Silenced 9mm
    weaponModel = 347, -- Silenced 9mm dff/txd
    toggleGuiKey = "b",
    deleteKey = "delete",
    ghostAlpha = 180,
    maxReachDistance = 50.0,
    stepMovement = 0.1,
    stepRotation = 5.0
}}

TOOLGUN_CATALOG = {{
    objects = {{
        items = {{
{chr(10).join(objects_items)}
        }}
    }},
    lights = {{
        items = {{
            {{ name = "Red Light", r = 255, g = 0, b = 0, radius = 10.0 }},
            {{ name = "Green Light", r = 0, g = 255, b = 0, radius = 10.0 }},
            {{ name = "Blue Light", r = 0, g = 0, b = 255, radius = 10.0 }},
            {{ name = "White Light", r = 255, g = 255, b = 255, radius = 10.0 }},
            {{ name = "Warm Light (Street)", r = 255, g = 180, b = 100, radius = 15.0 }},
            {{ name = "Neon Purple", r = 180, g = 0, b = 255, radius = 10.0 }},
            {{ name = "Police Sirens (Blue/Red)", r = 255, g = 0, b = 0, radius = 8.0 }}
        }}
    }},
    effects = {{
        items = {{
            {{ name = "Small Fire", effectName = "fire" }},
            {{ name = "Explosion", effectName = "explosion_large" }},
            {{ name = "Smoke", effectName = "smoke30lit" }},
            {{ name = "Sparks", effectName = "prt_spark_2" }},
            {{ name = "Water Splash", effectName = "water_splash" }}
        }}
    }}
}}
"""

with open(shared_lua_path, 'w', encoding='utf-8') as f:
    f.write(shared_lua_content)
print('Wrote shared.lua with', len(objects_items), 'objects')

with open(client_lua_path, 'r', encoding='utf-8') as f:
    client_code = f.read()

client_code = client_code.replace(
    'previewObject = createObject(modelId, 0, 0, -500)',
    'previewObject = createObject(modelId, 0, 0, -50)\n    setElementAlpha(previewObject, 255)'
)

old_pcall = """        local success, result = pcall(
            exports.object_preview.createObjectPreview,
            exports.object_preview,
            previewObject,
            0, 0, 0,
            absX, absY, absW, absH,
            false, true, true
        )"""
new_pcall = """        local success, result = pcall(function()
            return exports.object_preview:createObjectPreview(previewObject, 0, 0, 0, absX, absY, absW, absH, false, true)
        end)"""
client_code = client_code.replace(old_pcall, new_pcall)

client_code = client_code.replace('"guiComboBoxGetAccepted"', '"onClientGUIComboBoxAccepted"')

old_bind = """bindKey(TOOLGUN_CONFIG.toggleGuiKey, "down", function()
    toggleToolGunGUI()
end)"""
new_bind = """bindKey(TOOLGUN_CONFIG.toggleGuiKey, "down", function()
    if getPedWeapon(localPlayer) == TOOLGUN_CONFIG.weaponId then
        toggleToolGunGUI()
    else
        outputChatBox("#FF0000[Tool Gun] #FFFFFFBu menuyu acmak icin elinizde Tool Gun (Silenced 9mm) olmali!", 255, 255, 255, true)
    end
end)"""
client_code = client_code.replace(old_bind, new_bind)

with open(client_lua_path, 'w', encoding='utf-8') as f:
    f.write(client_code)
print('Patched client.lua')
