#ifndef LUACXX_ALGORITHM_INCLUDED
#define LUACXX_ALGORITHM_INCLUDED

#include "stack.hpp"

#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>

/*

=head1 NAME

algorithm.hpp - common algorithms and functions for Lua

=head1 SYNOPSIS

    #include <luacxx/algorithm.hpp>
    #include <luacxx/convert/numeric.hpp>

=head1 DESCRIPTION

*/

namespace lua {

/*

=head4 std::string str = lua::traceback(state, int toplevel);

Returns a Lua traceback for the given state. The toplevel will cause
the highest stack entries to be omitted, which is useful to ignore
debugging function calls.

*/

std::string traceback(lua_State* const state, const int toplevel);

/*

=head4 std::string str = lua::dump(state);

Returns a diagnostic representation of the Lua stack

    #include <luacxx/algorithm.hpp>
    #include <iostream>

    std::cerr << lua::dump(state) << std::endl;

*/

std::string dump(lua_State* const state);

std::string class_name(const lua::index& index);
std::string class_name(lua_State* const state, int pos);

const char* class_id(const lua::index& index);
const char* class_id(lua_State* const state, int pos);

size_t userdata_size(const lua::index& index);

template <class T>
struct is_type
{
    const bool is_same;

    is_type(const lua::index& index) :
        // Compare exactly for efficiency, but I really should
        // benchmark this to ensure the performance is worth it.
        is_same(lua::class_id(index) == required_name())
    {
    }

    is_type(lua_State* const state, const int pos) :
        is_type(lua::index(state, pos))
    {
    }

    const char* required_name() const
    {
        return lua::Metatable<T>::name;
    }

    operator bool() const
    {
        return is_same;
    }
};

/*

=head4 std::string str = lua::memory_address(index)

Returns a string of the memory address for the value at the given index.

    #include <luacxx/algorithm.hpp>
    #include <iostream>

    std::cerr << lua::memory_address(lua::index(state, -1)) << std::endl;

*/

std::string memory_address(const lua::index& index);

/*

=head4 lua::assert_type<Error>(const char* category, lua::type expected, lua::index)

Asserts that the stack value at the given index is of the expected type. If
not, then an error of given type is thrown with a provided message.

*/

template <class Error = lua::error>
void assert_type(const char* category, const lua::type& expected, const lua::index& given)
{
    if (given.type() == expected) {
        return;
    }
    std::stringstream str;
    str << category;
    str << ": ";
    str << "Lua stack value at index " << given.pos() << " must be a ";
    str << lua::type_info(expected).name();
    str << " but a " << given.type().name() << " was given instead.";
    throw Error(str.str());
}

/*

=head4 lua::index top = lua::top(state)

Returns the topmost stack index for the given state, as a lua::index.

*/

lua::index top(lua_State* const state);

/*

=head4 lua::size(state)

Returns lua_gettop(state).

*/

unsigned int size(lua_State* const state);

/*

=head4 lua::size(index)

Returns luaL_len for the given stack index.

*/
int size(const lua::index& index);

template <class T>
int size(T reference)
{
    auto rv = size(lua::push(reference.state(), reference));
    lua_pop(reference.state(), 1);
    return rv;
}

template <class T>
bool empty(T value)
{
    return lua::size(value) == 0;
}

/*

=head4 lua::remove(index)

Removes the stack index at the given position. Equivalent to
lua_remove

*/

void remove(const lua::index& target);

/*

=head4 lua::clear(state)

Removes all stack values from the given state. Equivalent to lua_settop(state,
0).

*/
void clear(lua_State* const state);

/*

=head4 lua::swap(lua::index, lua::index)

Swaps the two values at the given stack positions.

*/
void swap(const lua::index& a, const lua::index& b);

/*

=head4 lua::get_all<T>(std::vector<T>& results, lua::index first)

Takes all stack values from the given index to the top, and inserts them
into the given vector.

    auto name = lua::get<std::string>(state, 1);

    // Get the numbers for this name
    std::vector<int> nums;
    lua::get_all(nums, lua::index(state, 2));
*/

template <class T>
void get_all(std::vector<T>& results, lua::index index)
{
    while (index) {
        results.push_back(lua::get<T>(index++));
    }
}

template <class T>
void get_all(std::vector<T>& results, lua_State* const state, const int pos)
{
    return lua::get_all(results, lua::index(state, pos));
}

/*

=head2 Lua function invocation

All this stuff involves calling Lua from C++. This means converting C++
arguments into Lua values, handling errors during invocation, and
converting the returned value into something useful.

=head4 lua::invoke(lua::index callable)

Invokes the Lua function at the given stack index. The arguments are presumed
to be above the given index. All arguments along with the function will be
popped, and the return values will be pushed in their place.

    function sum(...)
        local total = 0;
        for i=1, select("#", ...) do
            local value = select(i, ...);
            total = total + value;
        end;

        return value;
    end;

    // And within C++

    auto callable = lua::push(state, lua::global(state, "sum"));
    lua::push(state, 1);
    lua::push(state, 2);
    lua::push(state, 3);
    lua::invoke(callable);

    assert(6 == lua::get<int>(state, -1));

Runtime errors from Lua will cause lua::errors to be thrown in C++. Other
errors, like memory errors, will cause std::runtime_errors to be thrown; these
are usually not recoverable.

*/

void invoke(const lua::index& callable);

/*

=head4 RV return_value = lua::call<RV>(source, args...)

Invokes the given source value with the given arguments. The returned value
will be converted and returned. The stack will be returned to its original
state.

    #include <luacxx/convert/string.hpp>
    #include <luacxx/algorithm.hpp>

    lua::call(env["print"], "Hello, world");

    std::cout << lua::call<std::string>(env["string"]["upper"], "Hello, world\n");
    // HELLO, WORLD
*/

template <typename RV, typename Callable, typename... Args>
RV call(Callable source, Args... args)
{
    lua::index callable(lua::push(source.state(), source));
    lua::assert_type("lua::call", lua::type::function, callable);
    lua::push(callable.state(), args...);
    lua::invoke(callable);

    lua_settop(callable.state(), callable.pos());
    return lua::get<RV>(callable.state(), callable.pos());
}

template <typename Callable, typename... Args>
void call(Callable source, Args... args)
{
    lua::index callable(lua::push(source.state(), source));
    lua::assert_type("lua::call", lua::type::function, callable);
    lua::push(callable.state(), args...);
    lua::invoke(callable);

    lua_settop(callable.state(), callable.pos() - 1);
}

template <class T>
lua::index push_global(lua_State* const state, const T&& name)
{
    auto globals = lua::push(state, lua::value::globals);
    auto rv = globals[name];
    lua::remove(globals);
    return --rv;
}

/*

=head4 int lua::make_malloc<T>(state)

Creates a new userdata and pushed it on the stack. The userdata's size is
determined by the specified type.

This returns 1, so it can be used directly as a callable Lua function for
simple C types, as shown in the example:

    env["chtype"] = lua::value::table;
    env["chtype"]["new"] = lua::make_malloc<chtype>;

*/

template <class T>
int make_malloc(lua_State* const state)
{
    lua::malloc(state, sizeof(T));
    return 1;
}

namespace table {

/*

=head4 lua::table::length(index)

Returns the size of the table at the given index.

*/

int length(const lua::index& index);

/*

=head4 lua::table::insert(destination, value)

Inserts a new value into the table at the given destination index.

    lua::table::insert(env["package"]["searchers"], my_search_function);

*/

template <typename Table, typename Value>
void insert(Table destination, Value value)
{
    auto table = lua::push(destination);
    lua::assert_type("lua::table::insert", lua::type::table, table);
    lua::push(table.state(), lua::table::length(table) + 1);
    lua::push(table.state(), value);

    lua_settable(table.state(), table.pos());

    lua_pop(table.state(), 1);
}

/*

=head4 Value lua::table::get<Value>(source, key)

Returns source[key] as a value of the given type. The Lua stack
is unaffected.

=head4 lua::index lua::table::get(source, key)

Pushes source[key] onto the Lua stack.

*/

template <typename Value, typename Table, typename Key>
Value get(Table source, Key key)
{
    auto table = lua::push(source);
    lua::assert_type("lua::table::get", lua::type::table, table);

    lua::push(table.state(), key);
    lua_gettable(table.state(), table.pos());
    lua_replace(table.state(), table.pos());

    auto rv = lua::get<Value>(table.state(), -1);
    lua_pop(table.state(), 1);
    return rv;
}

template <typename Table, typename Key>
lua::index get(Table source, Key key)
{
    auto table = lua::push(source);
    lua::assert_type("lua::table::get", lua::type::table, table);

    lua::push(table.state(), key);
    lua_gettable(table.state(), table.pos());
    lua_replace(table.state(), table.pos());

    return lua::index(table.state(), -1);
}

/*

=head4 lua::table::set(source, key, value)

Sets source[key] to the given value.

*/

template <typename Value, typename Key, typename Table>
void set(Table source, Key key, Value value)
{
    auto table = lua::push(source);
    lua::assert_type("lua::table::set", lua::type::table, table);

    lua::push(table.state(), key);
    lua::push(table.state(), value);
    lua_settable(table.state(), table.pos());

    lua_pop(table.state(), 1);
}

/*

=head4 lua::type_info rv = lua::table::get_type(source, key)

Returns the type of source[key], same as lua::index::type()

*/

template <typename Table, typename Key>
lua::type_info get_type(Table source, Key key)
{
    auto value = lua::table::get(source, key);
    auto rv = value.type();
    lua_pop(value.state(), 1);
    return rv;
}

} // namespace table

/*

=head4 lua::setfield(source, key, value)

Identical to lua::table::set(source, key, value)

*/

template <typename Value, typename Key, typename Table>
void setfield(Table source, Key key, Value value)
{
    lua::table::set(source, key, value);
}

/*

=head4 lua::setglobal<Value>(state, const char* name, Value value)

*/

template <typename Value>
void setglobal(lua_State* const state, const char* name, Value value)
{
    lua::push(state, value);
    lua_setglobal(state, name);
}

} // namespace lua

#endif // LUACXX_ALGORITHM_INCLUDED
