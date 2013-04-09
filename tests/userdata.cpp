#include "init.hpp"

#include "userdata.hpp"

BOOST_AUTO_TEST_CASE(testLuaHandleQObjects)
{
    Lua lua;
    lua.setAcceptsStackUserdata(true);
    LuaStack s(lua);

    QObject obj;
    s.push(&obj, "QObject");

    LuaUserdata* returned;
    s >> returned;

    BOOST_REQUIRE(returned->rawData() == &obj);
}

BOOST_AUTO_TEST_CASE(testLuaHandleQObjectsWithMinimalSemantics)
{
    Lua lua;
    LuaStack stack(lua);

    auto orig = std::shared_ptr<QObject>(new QObject);
    stack << orig;

    std::shared_ptr<QObject> other;
    stack >> other;

    BOOST_REQUIRE(other.get() == orig.get());
}

namespace lua {

template<>
struct UserdataName<Counter>
{
    constexpr static const char* value = "Counter";
};

template<>
struct UserdataName<Blank>
{
    constexpr static const char* value = "Blank";
};

} // namespace lua

LuaStack& operator<<(LuaStack& stack, Blank& ptr)
{
    stack << LuaUserdata(&ptr, "Blank");
    return stack;
}

LuaStack& operator<<(LuaStack& stack, Counter& ptr)
{
    stack << LuaUserdata(&ptr, "Counter");

    stack.pushMetatable();
    lua::userdata::qobject(stack, &ptr);
    stack.setMetatable();
    return stack;
}

BOOST_AUTO_TEST_CASE(luaRetrievesQObjectProperties)
{
    Lua lua;
    lua.setAcceptsStackUserdata(true);

    Counter counter(42);
    lua["counter"] = counter;
    lua("bar = counter.value");

    BOOST_REQUIRE_EQUAL(lua["bar"].as<int>(), 42);
}

BOOST_AUTO_TEST_CASE(luaCanSetQObjectProperties)
{
    Lua lua;
    lua.setAcceptsStackUserdata(true);

    Counter counter(42);
    lua["counter"] = counter;
    lua("counter.value = 24");

    BOOST_REQUIRE_EQUAL(counter.getValue(), 24);
}

static void receiveValue(Blank blank)
{
    BOOST_CHECK_EQUAL(blank.count(), 42);
}

static void receiveConstValue(const Blank blank)
{
    BOOST_CHECK_EQUAL(blank.count(), 42);
}

static void receivePtr(Counter* const counter)
{
    counter->setValue(24);
}

static int receiveConstPtr(const Counter* counter)
{
    return counter->getValue();
}

static void receiveRef(Counter& counter)
{
    counter.setValue(24);
}

static int receiveConstRef(const Counter& counter)
{
    return counter.getValue();
}

BOOST_AUTO_TEST_CASE(luaCanPassBackCxxValues)
{
    Lua lua;
    lua.setAcceptsStackUserdata(true);

    Counter counter(42);
    lua["counter"] = counter;
    lua["receiveValue"] = receiveValue;
    lua["receivePtr"] = receivePtr;
    lua["receiveRef"] = receiveRef;
    lua["receiveConstRef"] = receiveConstRef;
    lua["receiveConstPtr"] = receiveConstPtr;

    BOOST_CHECK_EQUAL(counter.getValue(), 42);
    lua("receivePtr(counter)");
    BOOST_CHECK_EQUAL(counter.getValue(), 24);
    BOOST_CHECK_EQUAL(lua("return receiveConstPtr(counter)").as<int>(), 24);

    counter.setValue(42);

    BOOST_CHECK_EQUAL(counter.getValue(), 42);
    lua("receiveRef(counter)");
    BOOST_CHECK_EQUAL(counter.getValue(), 24);
    BOOST_CHECK_EQUAL(lua("return receiveConstRef(counter)").as<int>(), 24);
}

BOOST_AUTO_TEST_CASE(throwLuaExceptionOnLuaProblems)
{
    Lua lua;
    lua["receiveRef"] = receiveRef;
    lua.setAcceptsStackUserdata(true);

    Counter counter(42);
    lua["counter"] = counter;

    // Syntax error
    BOOST_CHECK_THROW(lua("foo.bar.baz = 42"), LuaException);

    // Underflow
    BOOST_CHECK_THROW(lua("receiveRef()"), LuaException);

    // Allow extra args
    BOOST_CHECK_NO_THROW(lua("receiveRef(counter, 3)"));
}

BOOST_AUTO_TEST_CASE(luaCanPassUserdataByValue)
{
    Lua lua;
    lua.setAcceptsStackUserdata(true);

    Blank blank(42);
    lua["blank"] = blank;

    lua["receiveValue"] = receiveValue;
    lua["receiveConstValue"] = receiveConstValue;

    lua("receiveValue(blank)");
    lua("receiveConstValue(blank)");
}

void acceptShared(std::shared_ptr<Counter> ptr)
{
}

void acceptSharedRef(const std::shared_ptr<Counter>& ptr)
{
}

BOOST_AUTO_TEST_CASE(sharedPtrsCanBePassedToCxxFunctions)
{
    Lua lua;
    auto obj = std::shared_ptr<QObject>(new Counter(42));
    lua["c"] = obj;

    lua["acceptShared"] = acceptShared;
    lua["acceptSharedRef"] = acceptSharedRef;
}

BOOST_AUTO_TEST_CASE(qobjectDynamicallyAddsPropertiesWhenNonexistent)
{
    Lua lua;
    auto obj = std::shared_ptr<QObject>(new Counter(42));
    lua["c"] = obj;

    lua("c.someValue = 24");

    BOOST_REQUIRE_EQUAL((int)lua("return c.someValue"), 24);
}

BOOST_AUTO_TEST_CASE(luaCanCallQObjectMethods)
{
    Lua lua;
    lua["c"] = std::shared_ptr<QObject>(new Counter(42));
    lua("foo = c:getValue()");
    BOOST_REQUIRE(lua["foo"] == 42);
}

BOOST_AUTO_TEST_CASE(luaCanPassValuesToQObjectMethods)
{
    Lua lua;
    Counter* counter = new Counter;
    lua["c"] = std::shared_ptr<QObject>(counter);

    lua("c:setValue(24)");
    BOOST_REQUIRE_EQUAL(counter->getValue(), 24);
}

BOOST_AUTO_TEST_CASE(luaCanPassTwoValuesToQObjectMethods)
{
    Lua lua;
    Counter* counter = new Counter;
    lua["c"] = std::shared_ptr<QObject>(counter);

    lua("c:setAddedValue(3, 6)");
    BOOST_REQUIRE_EQUAL(counter->getValue(), 9);
}

BOOST_AUTO_TEST_CASE(methodsCanUseTheStackDirectly)
{
    Lua lua;
    Counter* counter = new Counter(2);
    lua["c"] = std::shared_ptr<QObject>(counter);

    lua("c:addAll(1, 2, 3)");
    BOOST_REQUIRE_EQUAL(counter->getValue(), 8);
}

BOOST_AUTO_TEST_CASE(methodsCanStillReturnValues)
{
    Lua lua;
    lua["c"] = std::shared_ptr<QObject>(new Counter(0));

    BOOST_REQUIRE_EQUAL((int)lua("return c:summed(1, 2, 3)"), 6);
}

BOOST_AUTO_TEST_CASE(luaSetsPropertiesDirectly)
{
    Lua lua;
    QFile file(LUA_DIR "anim.lua");
    lua(file);

    auto square = new Square;

    int old = square->getX();
    lua["Tick"](std::shared_ptr<QObject>(square), M_PI);
    BOOST_REQUIRE(square->getX() != old);
}

BOOST_AUTO_TEST_CASE(stackAcceptsRawPointers)
{
    Lua lua;

    Counter counter;
    LuaStack stack(lua);
    stack.setAcceptsStackUserdata(true);
    stack << counter;
    BOOST_REQUIRE_EQUAL(stack.typestring().c_str(), "userdata");
}

BOOST_AUTO_TEST_CASE(stackAllocatedObjectsAreAccepted)
{
    Lua lua;

    lua(
    "function work(a, b)"
    "   a.value = 50;"
    "   if b then b.value = 42; end;"
    "end;");

    Counter a(42);
    lua["work"](a, Counter(53));
    lua["work"](a, a);
    lua["work"](Counter(53), a);
    lua["work"](Counter(53), Counter(42));
    lua["work"](&a, Counter(42));
    lua["work"](&a, a);
    lua["work"](a, &a);

    //BOOST_REQUIRE_EQUAL(counter.getValue(), 50);
}
