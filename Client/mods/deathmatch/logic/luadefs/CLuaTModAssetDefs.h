#pragma once
#include "luadefs/CLuaDefs.h"

class CLuaTModAssetDefs : public CLuaDefs
{
public:
    static void LoadFunctions();

    static int LoadModel(lua_State* luaVM, std::string path);
    static int SpawnProp(lua_State* luaVM, std::string model, float x, float y, float z, float rx, float ry, float rz, float mass, bool isStatic);
    static bool SetPropPerma(lua_State* luaVM, int propId, bool perma);
};
