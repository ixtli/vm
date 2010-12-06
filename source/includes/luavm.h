#ifndef _LUAVM_H_
#define _LUAVM_H_

#include "global.h" // For size_t

enum LuaError {
    kLuaNoError         = 0,
    kLuaUnexpectedType  = 1,
    kLuaTableNotOpen    = 2,
    kLuaInvalidName     = 3
};

enum LuaFields {
    kLBool, kLDouble, kLUInt, kLInt, kLString
};

enum LuaConstants {
    kLuaMultiRet        = -1
};

struct lua_State;

class LuaVM
{
public:
    LuaVM();
    ~LuaVM();
    
    bool init();
    bool exec(const char *path, int results = kLuaMultiRet);
    
    // Retrieve values from global state
    LuaError getGlobalField(const char *name, LuaFields field, void *ret);
    LuaError getTableField(const char *name, LuaFields field, void *ret);
    LuaError getTableField(int index, LuaFields field, void *ret);
    size_t lengthOfCurrentObject();
    LuaError openTableAtTableIndex(int index);
    LuaError openGlobalTable(const char *name);
    LuaError closeTable();
    
private:
    LuaError getTopField(LuaFields field, void *ret);
    void reportErrors();
    lua_State *L;
};

#endif
