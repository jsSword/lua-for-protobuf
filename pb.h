#ifndef __PB_H__
#define __PB_H__

#include <lua.hpp>
#include <stdio.h>
#include "pbloader.h"

int init_pb(lua_State* l);

#define LUA_PB_MT	"pbmt"
#define LUA_PB_REPEATED_MT "pbrepeatedmt"
#ifdef _MSC_VER
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#define pb_check(l, exp, errmsg, ...) if(exp) { luaL_error(l, errmsg, __VA_ARGS__); return 0; }
#else
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#define pb_check(l, exp, errmsg, ...) if(exp) { luaL_error(l, errmsg, __VA_ARGS__); return 0; }
#endif // _MSC_VER

struct ProtoBufRepeatedField
{
	google::protobuf::Message* msg;
	const google::protobuf::FieldDescriptor* field;
};

#endif // __PB_H__
