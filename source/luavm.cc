#include "includes/luavm.h"

extern "C" {
    #include "includes/lua/lua.h"
    #include "includes/lua/lualib.h"
    #include "includes/lua/lauxlib.h"
}

LuaVM::LuaVM()
{}

LuaVM::~LuaVM()
{
    // Clean up lua
    lua_close(L);
}

void LuaVM::reportErrors()
{
    // Don't call this unless there /ARE/ errors.
    fprintf(stderr, "-- %s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
}

bool LuaVM::init()
{
    // Init the state
    L = lua_open();
    
    // Load the libs
    luaL_openlibs(L);
    
    return (false);
}

bool LuaVM::exec(const char *path, int results)
{
    // Error check
    if (!path) return (true);
    
    // This is the maximum amount of results expected from the function
    // we're about to call in protected mode.  They are pushed on the stack.
    int ret_count;
    // If results is less than 0, assume LUA_MULTRET, which means that ALL
    // results will be pushed to the stack
    if (results < kLuaMultiRet + 1)
        ret_count = LUA_MULTRET;
    else
        ret_count = results;
    
    // Load Lua script
    if (luaL_loadfile(L,path))
    {
        fprintf(stderr, "Error loading script '%s'.\n", path);
        return (true);
    }
    
    // Execute the script
    int err = lua_pcall(L, 0, ret_count, 0);
    if (err)
    {
        fprintf(stderr, "Error running script '%s':\n", path);
        reportErrors();
        return (true);
    }
    
    return (false);
}

// Error codes: 0 - No error
//              1 - invalid *name
//              2 - value named is not the right type
int LuaVM::getGlobalBool(const char *name, bool &ret)
{
    // Error check
    if (!name) return (1);
    
    lua_getfield(L, LUA_GLOBALSINDEX, name);
    if (!lua_isboolean(L, -1))
        return (2);
    
    ret = lua_toboolean(L, -1);
    lua_pop(L, 1);
    
    return (0);
}

int LuaVM::getGlobalDouble(const char *name, double &ret)
{
    // Error check
    if (!name) return (1);
    
    lua_getfield(L, LUA_GLOBALSINDEX, name);
    if (!lua_isnumber(L, -1))
        return (2);
    
    ret = lua_tonumber(L, -1);
    lua_pop(L, 1);
    
    return (0);
}

int LuaVM::getGlobalInt(const char *name, int &ret)
{
    // Error check
    if (!name) return (1);
    
    lua_getfield(L, LUA_GLOBALSINDEX, name);
    if (!lua_isnumber(L, -1))
        return (2);
    
    ret = lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    return (0);
}

int LuaVM::getGlobalUInt(const char *name, unsigned int &ret)
{
    // Error check
    if (!name) return (1);
    
    lua_getfield(L, LUA_GLOBALSINDEX, name);
    if (!lua_isnumber(L, -1))
        return (2);
    
    ret = (unsigned int) lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    return (0);
}

int LuaVM::getGlobalString(const char *name, const char **ret)
{
    // Error check
    if (!name) return (1);
    
    lua_getfield(L, LUA_GLOBALSINDEX, name);
    if (!lua_isstring(L, -1))
        return (2);
    
    *ret = lua_tostring(L, -1);
    lua_pop(L, 1);
    
    return (0);
}

