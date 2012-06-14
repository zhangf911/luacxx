#ifndef LUAGLOBAL_HPP
#define LUAGLOBAL_HPP

#include "LuaValue.hpp"
#include "LuaReference.hpp"
#include "LuaGlobalAccessible.hpp"

class LuaGlobal : public LuaValue
{
    const LuaGlobalAccessible _accessor;

protected:
    void push(LuaStack& stack) const
    {
        accessor().push(stack);
    }

    const LuaAccessible& accessor() const
    {
        return _accessor;
    }

public:
    LuaGlobal(Lua& lua, const QString& key) :
        LuaValue(lua),
        _accessor(lua, key)
    {}

    template<typename T>
    const LuaValue& operator=(const T& value)
    {
        LuaStack s(lua);
        s.push(value);
        accessor().store(s);
        return *this;
    }

    template <typename... Args>
    LuaReference operator()(Args... args)
    {
        LuaStack stack(lua);
        accessor.push(stack);
        return callLua(luaState(), stack, args...);
    }
};

#endif // LUAGLOBAL_HPP
