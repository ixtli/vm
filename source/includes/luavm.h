#ifndef _LUAVM_H_
#define _LUAVM_H_

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
    int getGlobalBool(const char *name, bool &ret);
    int getGlobalDouble(const char *name, double &ret);
    int getGlobalUInt(const char *name, unsigned int &ret);
    int getGlobalInt(const char *name, int &ret);
    int getGlobalString(const char *name, const char **ret);
    
private:
    void reportErrors();
    lua_State *L;
};

#endif
