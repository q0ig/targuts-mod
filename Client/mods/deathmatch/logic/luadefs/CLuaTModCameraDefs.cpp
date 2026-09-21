#include "StdInc.h"
#include "CLuaTModCameraDefs.h"
#include "../camera/CTModCameraManager.h"

void CLuaTModCameraDefs::LoadFunctions() {
    constexpr static const std::pair<const char*, lua_CFunction> functions[]{
        {"tmodSetCameraMode", ArgumentParser<SetCameraMode>},
        {"tmodSetCameraOffset", ArgumentParser<SetCameraOffset>},
        {"tmodSetCameraDistance", ArgumentParser<SetCameraDistance>},
        {"tmodGetCameraMode", ArgumentParser<GetCameraMode>}
    };

    for (const auto& [name, func] : functions)
        CLuaCFunctions::AddFunction(name, func);
}

bool CLuaTModCameraDefs::SetCameraMode(lua_State* luaVM, std::string mode) {
    CTModCameraManager::GetSingleton().SetCameraMode(mode);
    return true;
}

bool CLuaTModCameraDefs::SetCameraOffset(lua_State* luaVM, float x, float y, float z) {
    CTModCameraManager::GetSingleton().SetCameraOffset(x, y, z);
    return true;
}

bool CLuaTModCameraDefs::SetCameraDistance(lua_State* luaVM, float dist) {
    CTModCameraManager::GetSingleton().SetCameraDistance(dist);
    return true;
}

int CLuaTModCameraDefs::GetCameraMode(lua_State* luaVM) {
    std::string mode = CTModCameraManager::GetSingleton().GetCameraMode();
    lua_pushstring(luaVM, mode.c_str());
    return 1;
}
