#ifndef LUA_REFERENCE_ACCESSIBLE_HPP
#define LUA_REFERENCE_ACCESSIBLE_HPP

#include <lua.hpp>
#include "LuaAccessible.hpp"
#include "LuaEnvironment.hpp"
#include "LuaStack.hpp"

class LuaReferenceAccessible : public LuaAccessible
{
    int ref;
public:
    LuaReferenceAccessible(Lua& lua) : LuaAccessible(lua)
    {
        ref = luaL_ref(luaState(), LUA_REGISTRYINDEX);
    }

    void push(LuaStack&) const
    {
        lua_rawgeti(luaState(), LUA_REGISTRYINDEX, ref);
    }

    void store(LuaStack& stack) const
    {
        lua_rawseti(luaState(), LUA_REGISTRYINDEX, ref);
        // Push it again to leave the stack unaffected by this
        // action.
        push(stack);
    }

    ~LuaReferenceAccessible()
    {
        luaL_unref(luaState(), LUA_REGISTRYINDEX, ref);
    }
};

#endif // LUA_REFERENCE_ACCESSIBLE_HPP
