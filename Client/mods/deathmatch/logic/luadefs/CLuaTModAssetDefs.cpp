#include "StdInc.h"
#include "CLuaTModAssetDefs.h"
#include "../assets/CTModAssetManager.h"
#include "CScriptArgReader.h"

void CLuaTModAssetDefs::LoadFunctions() {
    constexpr static const std::pair<const char*, lua_CFunction> functions[]{
        {"tmodLoadModel", ArgumentParser<LoadModel>},
        {"tmodSpawnProp", ArgumentParser<SpawnProp>},
        {"tmodSetPropPerma", ArgumentParser<SetPropPerma>}
    };

    for (const auto& [name, func] : functions)
        CLuaCFunctions::AddFunction(name, func);
}

int CLuaTModAssetDefs::LoadModel(lua_State* luaVM, std::string path) {
    std::string handle = CTModAssetManager::GetSingleton().LoadModel(path);
    if (!handle.empty()) {
        if (g_pCore && g_pCore->GetConsole()) {
            g_pCore->GetConsole()->Printf("[TMod] Loaded model successfully: %s", path.c_str());
        }
    } else {
        if (g_pCore && g_pCore->GetConsole()) {
            g_pCore->GetConsole()->Printf("[TMOD-ERROR] Failed to load model: %s", path.c_str());
        }
    }
    lua_pushstring(luaVM, handle.c_str());
    return 1;
}

int CLuaTModAssetDefs::SpawnProp(lua_State* luaVM, std::string model, float x, float y, float z, float rx, float ry, float rz, float mass, bool isStatic) {
    int id = CTModAssetManager::GetSingleton().SpawnProp(model, x, y, z, rx, ry, rz, mass, isStatic);
    if (id != -1) {
        if (g_pCore && g_pCore->GetConsole()) {
            g_pCore->GetConsole()->Printf("[TMod] FBX map/prop spawned successfully at %.2f, %.2f, %.2f", x, y, z);
        }
    } else {
        if (g_pCore && g_pCore->GetConsole()) {
            g_pCore->GetConsole()->Printf("[TMOD-ERROR] Failed to spawn prop. Model not loaded: %s", model.c_str());
        }
    }
    lua_pushinteger(luaVM, id);
    return 1;
}

bool CLuaTModAssetDefs::SetPropPerma(lua_State* luaVM, int propId, bool perma) {
    CTModAssetManager::GetSingleton().SetPropPerma(propId, perma);
    return true;
}
