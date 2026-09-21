#pragma once
#include "luadefs/CLuaDefs.h"

class CLuaTModMovementDefs : public CLuaDefs
{
public:
    static void LoadFunctions();

    static bool SetMovementMode(lua_State* luaVM, std::string mode);
    static bool SetAirAccelerate(lua_State* luaVM, float val);
    static bool SetAutoBhop(lua_State* luaVM, bool b);
    static int GetMovementMode(lua_State* luaVM);
};
