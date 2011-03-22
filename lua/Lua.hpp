#ifndef HEADER_LUA_HPP
#define HEADER_LUA_HPP

#include <string>
#include <istream>
#include <lua.hpp>

#include "LuaValue.hpp"

using std::istream;
using std::string;

class Lua
{
private:
public:
	// XXX make this private once testing is complete
	lua_State * state;
	Lua();
	~Lua();

	LuaValue operator[](const string& key);

	void operator()(istream& stream, const char* name);
	void operator()(const string& input);

	friend class LuaValue;
};

#endif