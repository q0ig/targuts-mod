#pragma once
#include "luadefs/CLuaDefs.h"

class CLuaTModCameraDefs : public CLuaDefs
{
public:
    static void LoadFunctions();

    static bool SetCameraMode(lua_State* luaVM, std::string mode);
    static bool SetCameraOffset(lua_State* luaVM, float x, float y, float z);
    static bool SetCameraDistance(lua_State* luaVM, float dist);
    static int GetCameraMode(lua_State* luaVM);
};
