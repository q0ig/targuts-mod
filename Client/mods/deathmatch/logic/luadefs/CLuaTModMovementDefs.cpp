#include "StdInc.h"
#include "CLuaTModMovementDefs.h"
#include "../movement/CTModMovementManager.h"
#include "CScriptArgReader.h"

void CLuaTModMovementDefs::LoadFunctions() {
    constexpr static const std::pair<const char*, lua_CFunction> functions[]{
        {"tmodSetMovementMode", ArgumentParser<SetMovementMode>},
        {"tmodSetAirAccelerate", ArgumentParser<SetAirAccelerate>},
        {"tmodSetAutoBhop", ArgumentParser<SetAutoBhop>},
        {"tmodGetMovementMode", ArgumentParser<GetMovementMode>}
    };

    for (const auto& [name, func] : functions)
        CLuaCFunctions::AddFunction(name, func);
}

bool CLuaTModMovementDefs::SetMovementMode(lua_State* luaVM, std::string mode) {
    CTModMovementManager::GetSingleton().SetMovementMode(mode);
    return true;
}

bool CLuaTModMovementDefs::SetAirAccelerate(lua_State* luaVM, float val) {
    CTModMovementManager::GetSingleton().SetAirAccelerate(val);
    return true;
}

bool CLuaTModMovementDefs::SetAutoBhop(lua_State* luaVM, bool b) {
    CTModMovementManager::GetSingleton().SetAutoBhop(b);
    return true;
}

int CLuaTModMovementDefs::GetMovementMode(lua_State* luaVM) {
    std::string mode = CTModMovementManager::GetSingleton().GetMovementMode();
    lua_pushstring(luaVM, mode.c_str());
    return 1;
}
