#pragma once
#include "luadefs/CLuaDefs.h"

class CLuaTModCameraDefs : public CLuaDefs
{
public:
    static void LoadFunctions();

    static bool SetCameraMode(lua_State* luaVM, std::string mode);
    static bool SetCameraOffset(lua_State* luaVM, float x, float y, float z);
    static bool SetCameraDistance(lua_State* luaVM, float dist);
    static int  GetCameraMode(lua_State* luaVM);

    // Viewmodel / Silah ve El kontrolleri
    static bool SetWeaponViewmodel(lua_State* luaVM, int weaponType, std::string fbxPath, std::string texturePath);
    static bool SetViewmodelOffset(lua_State* luaVM, float right, float forward, float down, float pitchDeg);
    static bool SetViewmodelScale(lua_State* luaVM, float scaleForward, float scaleUp, float scaleRight);
    static bool SetViewmodelAnimInterval(lua_State* luaVM, float intervalSeconds);
    static bool SetViewmodelAnimName(lua_State* luaVM, std::string animName);
};
