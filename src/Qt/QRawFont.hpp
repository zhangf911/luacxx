#ifndef LUA_CXX_QRAWFONT_INCLUDED
#define LUA_CXX_QRAWFONT_INCLUDED

#include "../stack.hpp"

class QRawFont;

namespace lua {

void QRawFont_metatable(const lua::index& mt);

template <>
struct Metatable<QRawFont>
{
    static constexpr const char* name = "QRawFont";

    static bool metatable(const lua::index& mt, QRawFont* const)
    {
        lua::QRawFont_metatable(mt);
        return true;
    }
};

}; // namespace lua

extern "C" int luaopen_luacxx_QRawFont(lua::state* const);

#endif // LUA_CXX_QRAWFONT_INCLUDED
