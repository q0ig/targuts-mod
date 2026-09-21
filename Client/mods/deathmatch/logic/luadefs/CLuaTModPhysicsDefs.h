#pragma once
#include "luadefs/CLuaDefs.h"
#include <optional>

class CClientEntity;

class CLuaTModPhysicsDefs : public CLuaDefs
{
public:
    static void LoadFunctions();

    // Lua calls
    static int  CreateRigidBody(lua_State* luaVM, float x, float y, float z, float sx, float sy, float sz, float mass);
    static int  AttachPhysics(lua_State* luaVM, CClientEntity* pEntity, std::optional<float> mass, std::optional<float> sx, std::optional<float> sy,
                              std::optional<float> sz);
    static bool DetachPhysics(lua_State* luaVM, CClientEntity* pEntity);
    static bool BindBodyToElement(lua_State* luaVM, int id, CClientEntity* pEntity);
    static bool DestroyBody(lua_State* luaVM, int id);
    static bool SetBodyPosition(lua_State* luaVM, int id, float x, float y, float z);
    static int  GetBodyPosition(lua_State* luaVM, int id);
    static int  GetBodyRotation(lua_State* luaVM, int id);
    static int  GetBodyVelocity(lua_State* luaVM, int id);
    static bool SetBodyVelocity(lua_State* luaVM, int id, float vx, float vy, float vz);
    static bool ApplyForce(lua_State* luaVM, int id, float fx, float fy, float fz);
    static bool FreezeBody(lua_State* luaVM, int id, bool bFreeze);
    static bool IsBodyFrozen(lua_State* luaVM, int id);
};
