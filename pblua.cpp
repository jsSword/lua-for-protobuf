#include "pbloader.h"
#include <lua.hpp>
#include "pb.h"

#ifdef _MSC_VER
#define EXPROT_API extern "C" __declspec(dllexport)
#else
#define EXPROT_API extern "C"
#endif // _MSC_VER

//pbloader
static int pbloader_loadfile(lua_State* l)
{
	luaL_argcheck(l, lua_gettop(l) == 1, 1, "expected 1 argument");
	if (!lua_isstring(l, 1)) {
		luaL_error(l, "expected string argument\n");
		return 0;
	}

	size_t str_len;
	const char* str = lua_tolstring(l, 1, &str_len);
	pb_check(l, str == NULL, "load:parsed string is null!\n");

	if (g_pProtoLoader->ImportFile(std::string(str, str_len))) {
		luaL_error(l, "import file failed!\n");
		return 0;
	}
	return 0;
}

static int pbloader_loaddir(lua_State* l)
{

	return 0;
}

static int pbloader_create(lua_State* l)
{
	luaL_argcheck(l, lua_gettop(l) == 1, 1, "expected 1 argument");
	
	 const char* name = lua_tostring(l, 1);
	 google::protobuf::Message* pMessage = g_pProtoLoader->CreateDynMessage(name);
	 pb_check(l, pMessage == NULL, "message:%s does not exist!\n", name);

	 void* p = lua_newuserdata(l, sizeof(google::protobuf::Message*));
	 *(google::protobuf::Message **)p = pMessage;

	 luaL_setmetatable(l, LUA_PB_MT);
	 return 1;
}

static int pbloader_mappath(lua_State* l)
{
	luaL_argcheck(l, lua_gettop(l) == 2, 1, "expected 1 argument");
	if (!lua_isstring(l, 1) || !lua_isstring(l, 2)) {
		luaL_error(l, "expected the argument is string\n");
		return 0;
	}

	size_t str_len;
	const char* str = lua_tolstring(l, 1, &str_len);
	pb_check(l, str == NULL, "mpath: parsed string 1th is null!\n");

	std::string virpath = std::string(str, str_len);
	const char* str2 = lua_tolstring(l, 2, &str_len);
	pb_check(l, str == NULL, "mpath: parsed string 2th is null!\n");

	std::string diskpath = std::string(str2, str_len);
	g_pProtoLoader->MapPath(virpath, diskpath);
	return 0;
}

unsigned char hexchar[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
static int pb_hex(lua_State* l)
{
	luaL_argcheck(l, lua_gettop(l) == 1, 1, "expected 2 argument");
	pb_check(l, !lua_isstring(l, 1), "expected a string!");
	size_t str_len;
	const char* str = lua_tolstring(l, 1, &str_len);
	luaL_Buffer lb;
	luaL_buffinitsize(l, &lb, str_len * 3);

	for (int i = 0; i < str_len; ++i) {
		unsigned char c = str[i];
		luaL_addchar(&lb, hexchar[(c >> 4)]);
		luaL_addchar(&lb, hexchar[(c & 0x0F)]);
		luaL_addchar(&lb, (i == str_len - 1) ? '\0' : ' ');
	}
	luaL_pushresult(&lb);
	return 1;
}

EXPROT_API int luaopen_pblua(lua_State* l)
{
	init_pb(l);
	luaL_Reg pbreg[] = {
		{ "mpath", pbloader_mappath },
		{ "load", pbloader_loadfile },
		{ "loaddir", pbloader_loaddir },
		{ "create", pbloader_create },
		{ "hex", pb_hex},
		{ NULL, NULL },
	};
	lua_newtable(l);
	luaL_setfuncs(l, pbreg, 0);

	return 1;
}


int main(int argc, char *argv[])
{
	lua_State* L = luaL_newstate();
	luaopen_pblua(L);
	g_pProtoLoader->MapPath("", "H:\\code\\c++\\pb\\proto");
	g_pProtoLoader->ImportFile("orng.proto");

	google::protobuf::Message* msg = g_pProtoLoader->CreateDynMessage("orng");
	const google::protobuf::Descriptor* desc = msg->GetDescriptor();

	const google::protobuf::Reflection* ref = msg->GetReflection();
	const google::protobuf::FieldDescriptor* color_desc = desc->FindFieldByName("color");
	ref->SetInt32(msg, color_desc, 12);

	std::cout << msg->DebugString() << std::endl;

	return 0;
}
