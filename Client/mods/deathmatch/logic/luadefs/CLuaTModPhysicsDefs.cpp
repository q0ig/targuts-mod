#include "StdInc.h"
#include "CLuaTModPhysicsDefs.h"
#include "../physics/CTModPhysicsManager.h"
#include "CScriptArgReader.h"

/**
 * CLuaTModPhysicsDefs
 *
 * Bullet Physics C++ motorunu Lua ortamına bağlayan köprü sınıfı.
 *
 * Neden var?
 * Lua betiklerinin (özellikle GMod/TMOD prop spawn ve physgun sistemlerinin)
 * Bullet motorunda rijit gövde oluşturabilmesi, GTA nesnelerine (obje ve araç)
 * Bullet fiziği bağlayabilmesi, kuvvet uygulayabilmesi ve konum/rotasyon sorgulayabilmesi
 * için gerekli tüm API'leri sunar.
 */

void CLuaTModPhysicsDefs::LoadFunctions()
{
    constexpr static const std::pair<const char*, lua_CFunction> functions[]{
        {"tmodCreateRigidBody", ArgumentParser<CreateRigidBody>}, {"tmodAttachPhysics", ArgumentParser<AttachPhysics>},
        {"tmodDetachPhysics", ArgumentParser<DetachPhysics>},     {"tmodBindBodyToElement", ArgumentParser<BindBodyToElement>},
        {"tmodDestroyBody", ArgumentParser<DestroyBody>},         {"tmodSetBodyPosition", ArgumentParser<SetBodyPosition>},
        {"tmodGetBodyPosition", ArgumentParser<GetBodyPosition>}, {"tmodGetBodyRotation", ArgumentParser<GetBodyRotation>},
        {"tmodGetBodyVelocity", ArgumentParser<GetBodyVelocity>}, {"tmodSetBodyVelocity", ArgumentParser<SetBodyVelocity>},
        {"tmodApplyForce", ArgumentParser<ApplyForce>},           {"tmodFreezeBody", ArgumentParser<FreezeBody>},
        {"tmodIsBodyFrozen", ArgumentParser<IsBodyFrozen>}};

    // Add functions to Lua Manager
    for (const auto& [name, func] : functions)
        CLuaCFunctions::AddFunction(name, func);
}

int CLuaTModPhysicsDefs::CreateRigidBody(lua_State* luaVM, float x, float y, float z, float sx, float sy, float sz, float mass)
{
    int bodyId = CTModPhysicsManager::GetSingleton().CreateRigidBody(x, y, z, sx, sy, sz, mass);
    lua_pushinteger(luaVM, bodyId);
    return 1;
}

int CLuaTModPhysicsDefs::AttachPhysics(lua_State* luaVM, CClientEntity* pEntity, std::optional<float> mass, std::optional<float> sx, std::optional<float> sy,
                                       std::optional<float> sz)
{
    if (!pEntity)
        return 0;
    int bodyId = CTModPhysicsManager::GetSingleton().AttachPhysics(pEntity, mass.value_or(0.0f), sx.value_or(0.0f), sy.value_or(0.0f), sz.value_or(0.0f));
    if (bodyId > 0)
    {
        lua_pushinteger(luaVM, bodyId);
        return 1;
    }
    return 0;
}

bool CLuaTModPhysicsDefs::DetachPhysics(lua_State* luaVM, CClientEntity* pEntity)
{
    return CTModPhysicsManager::GetSingleton().DetachPhysics(pEntity);
}

bool CLuaTModPhysicsDefs::BindBodyToElement(lua_State* luaVM, int id, CClientEntity* pEntity)
{
    return CTModPhysicsManager::GetSingleton().BindBodyToElement(id, pEntity);
}

bool CLuaTModPhysicsDefs::DestroyBody(lua_State* luaVM, int id)
{
    CTModPhysicsManager::GetSingleton().DestroyBody(id);
    return true;
}

bool CLuaTModPhysicsDefs::SetBodyPosition(lua_State* luaVM, int id, float x, float y, float z)
{
    CTModPhysicsManager::GetSingleton().SetBodyPosition(id, x, y, z);
    return true;
}

int CLuaTModPhysicsDefs::GetBodyPosition(lua_State* luaVM, int id)
{
    float x = 0.0f, y = 0.0f, z = 0.0f;
    if (CTModPhysicsManager::GetSingleton().GetBodyPosition(id, x, y, z))
    {
        lua_pushnumber(luaVM, x);
        lua_pushnumber(luaVM, y);
        lua_pushnumber(luaVM, z);
        return 3;
    }
    return 0;
}

int CLuaTModPhysicsDefs::GetBodyRotation(lua_State* luaVM, int id)
{
    float rx = 0.0f, ry = 0.0f, rz = 0.0f;
    if (CTModPhysicsManager::GetSingleton().GetBodyRotation(id, rx, ry, rz))
    {
        lua_pushnumber(luaVM, rx);
        lua_pushnumber(luaVM, ry);
        lua_pushnumber(luaVM, rz);
        return 3;
    }
    return 0;
}

int CLuaTModPhysicsDefs::GetBodyVelocity(lua_State* luaVM, int id)
{
    float vx = 0.0f, vy = 0.0f, vz = 0.0f;
    if (CTModPhysicsManager::GetSingleton().GetBodyVelocity(id, vx, vy, vz))
    {
        lua_pushnumber(luaVM, vx);
        lua_pushnumber(luaVM, vy);
        lua_pushnumber(luaVM, vz);
        return 3;
    }
    return 0;
}

bool CLuaTModPhysicsDefs::SetBodyVelocity(lua_State* luaVM, int id, float vx, float vy, float vz)
{
    CTModPhysicsManager::GetSingleton().SetBodyVelocity(id, vx, vy, vz);
    return true;
}

bool CLuaTModPhysicsDefs::ApplyForce(lua_State* luaVM, int id, float fx, float fy, float fz)
{
    CTModPhysicsManager::GetSingleton().ApplyForce(id, fx, fy, fz);
    return true;
}

bool CLuaTModPhysicsDefs::FreezeBody(lua_State* luaVM, int id, bool bFreeze)
{
    CTModPhysicsManager::GetSingleton().FreezeBody(id, bFreeze);
    return true;
}

bool CLuaTModPhysicsDefs::IsBodyFrozen(lua_State* luaVM, int id)
{
    return CTModPhysicsManager::GetSingleton().IsBodyFrozen(id);
}
