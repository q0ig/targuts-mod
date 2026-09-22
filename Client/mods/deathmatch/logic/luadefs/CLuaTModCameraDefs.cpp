#include "StdInc.h"
#include "CLuaTModCameraDefs.h"
#include "../camera/CTModCameraManager.h"
#include "../rendering/CTModViewmodelManager.h"

void CLuaTModCameraDefs::LoadFunctions()
{
    constexpr static const std::pair<const char*, lua_CFunction> functions[]{
        {"tmodSetCameraMode", ArgumentParser<SetCameraMode>},
        {"tmodSetCameraOffset", ArgumentParser<SetCameraOffset>},
        {"tmodSetCameraDistance", ArgumentParser<SetCameraDistance>},
        {"tmodGetCameraMode", ArgumentParser<GetCameraMode>},

        // Viewmodel / Silah ve El Yapılandırması
        {"tmodSetWeaponViewmodel", ArgumentParser<SetWeaponViewmodel>},
        {"tmodSetViewmodelOffset", ArgumentParser<SetViewmodelOffset>},
        {"tmodSetViewmodelScale", ArgumentParser<SetViewmodelScale>},
        {"tmodSetViewmodelAnimInterval", ArgumentParser<SetViewmodelAnimInterval>},
        {"tmodSetViewmodelAnimName", ArgumentParser<SetViewmodelAnimName>},
    };

    for (const auto& [name, func] : functions)
        CLuaCFunctions::AddFunction(name, func);
}

bool CLuaTModCameraDefs::SetCameraMode(lua_State* luaVM, std::string mode)
{
    CTModCameraManager::GetSingleton().SetCameraMode(mode);
    return true;
}

bool CLuaTModCameraDefs::SetCameraOffset(lua_State* luaVM, float x, float y, float z)
{
    CTModCameraManager::GetSingleton().SetCameraOffset(x, y, z);
    return true;
}

bool CLuaTModCameraDefs::SetCameraDistance(lua_State* luaVM, float dist)
{
    CTModCameraManager::GetSingleton().SetCameraDistance(dist);
    return true;
}

int CLuaTModCameraDefs::GetCameraMode(lua_State* luaVM)
{
    std::string mode = CTModCameraManager::GetSingleton().GetCameraMode();
    lua_pushstring(luaVM, mode.c_str());
    return 1;
}

bool CLuaTModCameraDefs::SetWeaponViewmodel(lua_State* luaVM, int weaponType, std::string fbxPath, std::string texturePath)
{
    CTModViewmodelManager::GetSingleton().SetWeaponViewmodel(weaponType, fbxPath, texturePath);
    return true;
}

bool CLuaTModCameraDefs::SetViewmodelOffset(lua_State* luaVM, float right, float forward, float down, float pitchDeg)
{
    CTModViewmodelManager::GetSingleton().SetViewmodelOffset(right, forward, down, pitchDeg);
    return true;
}

bool CLuaTModCameraDefs::SetViewmodelScale(lua_State* luaVM, float scaleForward, float scaleUp, float scaleRight)
{
    CTModViewmodelManager::GetSingleton().SetViewmodelScale(scaleForward, scaleUp, scaleRight);
    return true;
}

bool CLuaTModCameraDefs::SetViewmodelAnimInterval(lua_State* luaVM, float intervalSeconds)
{
    CTModViewmodelManager::GetSingleton().SetAnimInterval(intervalSeconds);
    return true;
}

bool CLuaTModCameraDefs::SetViewmodelAnimName(lua_State* luaVM, std::string animName)
{
    CTModViewmodelManager::GetSingleton().SetAnimName(animName);
    return true;
}
