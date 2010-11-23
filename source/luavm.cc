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

LuaError LuaVM::getTopField(LuaFields field, void *ret)
{
    LuaError err = kLuaNoError;
    
    // Check to make sure the request is what we think it is
    switch (field)
    {
        case kLBool:
        if (!lua_isboolean(L, -1))
            err = kLuaUnexpectedType;
        else
            *((bool *)ret) = lua_toboolean(L, -1);
        break;
        
        case kLString:
        if (!lua_isstring(L, -1))
            err = kLuaUnexpectedType;
        else 
            *((const char **)ret) = lua_tostring(L, -1);
        break;
        
        case kLUInt:
        case kLInt:
        case kLDouble:
        default:
        if (!lua_isnumber(L, -1))
        {
            err = kLuaUnexpectedType;
        } else {
            switch (field)
            {
                case kLUInt:
                *((unsigned int *)ret) = (unsigned int) lua_tointeger(L, -1);
                break;
                
                case kLDouble:
                *((double *)ret) = lua_tonumber(L, -1);
                break;
                
                case kLInt:
                default:
                *((int *)ret) = lua_tointeger(L, -1);
                break;
            }
        }
        break;
    }
    
    return (err);
}

LuaError LuaVM::getGlobalField(const char *name, LuaFields field, void *ret)
{
    // Error check
    if (!name) return (kLuaInvalidName);
    
    // Request field from lua
    lua_getfield(L, LUA_GLOBALSINDEX, name);
    
    // Process the field
    LuaError err = getTopField(field, ret);
    
    // Clean up and exit
    lua_pop(L, 1);
    return (err);
}

LuaError LuaVM::getTableField(const char *name, LuaFields field, void *ret)
{
    if (!name) return (kLuaInvalidName);
    
    if (!lua_istable(L, -1))
        return (kLuaTableNotOpen);
    
    // Push the parameter on the stack, in this case the name of the key
    // in the Lua table
    lua_pushstring(L, name);
    // Get the value for the key.  The key is popped and the result is pushed.
    lua_gettable(L, -2);
    
    // Result is now at (L, -1)
    LuaError err = getTopField(field, ret);
    
    // Clean up the result and exit
    lua_pop(L, 1);
    return (err);
}

LuaError LuaVM::getTableField(int index, LuaFields field, void *ret)
{
    if (!lua_istable(L, -1))
        return (kLuaTableNotOpen);
    
    // Push the index
    lua_pushinteger(L, index);
    // Get the value for the index.
    lua_gettable(L, -2);
    
    // Result is now at (L, -1)
    LuaError err = getTopField(field, ret);
    
    // Clean and return
    lua_pop(L, 1);
    return (err);
}

LuaError LuaVM::openGlobalTable(const char *name)
{
    if (!name) return (kLuaInvalidName);
    
    lua_getfield(L, LUA_GLOBALSINDEX, name);
    if (!lua_istable(L, -1))
        return (kLuaUnexpectedType);
    
    return (kLuaNoError);
}

LuaError LuaVM::closeTable()
{
    if (!lua_istable(L, -1))
        return (kLuaTableNotOpen);
    
    lua_pop(L, 1);
    
    return (kLuaNoError);
}

size_t LuaVM::lengthOfCurrentObject()
{
    return (lua_objlen(L, -1));
}
