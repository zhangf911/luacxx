#include "QElapsedTimer.hpp"
#include "../type/function.hpp"
#include "../type/numeric.hpp"
#include "../thread.hpp"

#include <QElapsedTimer>

void lua::QElapsedTimer_metatable(const lua::index& mt)
{
    mt["elapsed"] = &QElapsedTimer::elapsed;
    mt["hasExpired"] = &QElapsedTimer::hasExpired;
    mt["invalidate"] = &QElapsedTimer::invalidate;
    mt["isValid"] = &QElapsedTimer::isValid;
    mt["msecsSinceReference"] = &QElapsedTimer::msecsSinceReference;
    mt["msecsTo"] = &QElapsedTimer::msecsTo;
    mt["nsecsElapsed"] = &QElapsedTimer::nsecsElapsed;
    mt["restart"] = &QElapsedTimer::restart;
    mt["secsTo"] = &QElapsedTimer::secsTo;
    mt["start"] = &QElapsedTimer::start;
}

int QElapsedTimer_new(lua::state* const state)
{
    lua::make<QElapsedTimer>(state);
    return 1;
}

int luaopen_luacxx_QElapsedTimer(lua::state* const state)
{
    lua::thread env(state);

    env["QElapsedTimer"] = lua::value::table;
    env["QElapsedTimer"]["new"] = QElapsedTimer_new;
    env["QElapsedTimer"]["isMonotonic"] = &QElapsedTimer::isMonotonic;
    //env["QElapsedTimer"]["clockType"] = &QElapsedTimer::clockType;

    return 0;
}
