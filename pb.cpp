#include "pb.h"
#include <stdio.h>

#ifdef _MSC_VER
#pragma warning(disable:4244)
#endif // _MSC_VER

#define pb_tomsg(l, idx) (*(google::protobuf::Message** )luaL_checkudata(l, idx, LUA_PB_MT))
#define pb_torepeated(l, idx) ((ProtoBufRepeatedField *)luaL_checkudata(l, idx, LUA_PB_REPEATED_MT))

static int pb_encode(lua_State* l)
{
	luaL_argcheck(l, lua_gettop(l) == 1, 1, "expected 2 argument");
	google::protobuf::Message* pMessage = pb_tomsg(l, 1);
	pb_check(l, pMessage == NULL, "got a nil msg\n");

	std::string str;
	if (pMessage->SerializeToString(&str) == false) {
		lua_pushnil(l);
		return 1;
	}
	lua_pushlstring(l, str.c_str(), str.size());
	return 1;
}

static int pb_decode(lua_State* l)
{
	luaL_argcheck(l, lua_gettop(l) == 2, 1, "expected 2 argument");
	google::protobuf::Message* pMessage = pb_tomsg(l, 1);
	pb_check(l, pMessage == NULL, "got a nil msg\n");
	pb_check(l, !lua_isstring(l, 2), "expected the 2th argument is string\n");

	size_t str_len;
	const char* str = lua_tolstring(l, 2, &str_len);
	if (!str) {
		LOG("parsed string is null\n");
		return 0;
	}

	if (!pMessage->ParseFromArray(str, (int)str_len)) {
		LOG("ParseFromArray failed\n");
		return 0;
	}

	lua_pushboolean(l, true);
	return 1;
}

static int pb_tostring(lua_State* l)
{
	luaL_argcheck(l, lua_gettop(l) == 1, 1, "expected 1 argument");
	google::protobuf::Message* pMessage = pb_tomsg(l, 1);
	pb_check(l, pMessage == NULL, "got a nil msg\n");
	std::string str = pMessage->DebugString();
	lua_pushlstring(l, str.c_str(), str.size());
	return 1;
}

static int pb_field(lua_State* l)
{
	luaL_argcheck(l, lua_gettop(l) == 2, 1, "expected 1 argument");

	google::protobuf::Message* pMessage = pb_tomsg(l, 1);
	pb_check(l, pMessage == NULL, "got a nil msg\n");

	const char* pFieldName = luaL_checkstring(l, 2);
	const google::protobuf::Descriptor* pDescriptor = pMessage->GetDescriptor();
	pb_check(l, pDescriptor == NULL, "descriptor is nil\n");

	const google::protobuf::Reflection* pReflection = pMessage->GetReflection();
	pb_check(l, pReflection == NULL, "reflection is nil\n");

	const google::protobuf::FieldDescriptor* pField = pDescriptor->FindFieldByName(pFieldName);
	pb_check(l, pField == NULL, "msg:%s, without field:%s \n", pDescriptor->full_name().c_str(), pFieldName);

	//repeated
	if (pField->is_repeated()) {
		ProtoBufRepeatedField* pRepeatedField = (ProtoBufRepeatedField*)lua_newuserdata(l, sizeof(ProtoBufRepeatedField));
		pb_check(l, pRepeatedField == NULL, "lua_newuserdata failed! %s.%s \n", pDescriptor->full_name().c_str(), pFieldName);
		pRepeatedField->msg = pMessage;
		pRepeatedField->field = pField;
		luaL_setmetatable(l, LUA_PB_REPEATED_MT);
		return 1;
	}

	switch(pField->cpp_type()) {
	case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
		google::protobuf::Message* pNewMessage = pReflection->MutableMessage(pMessage, pField);
		if (pNewMessage == NULL) {
			LOG("pNewMessage is nil\n");
			return 0;
		}
		void* p = lua_newuserdata(l, sizeof(google::protobuf::Message*));
		*(google::protobuf::Message**)p = pNewMessage;
		luaL_setmetatable(l, LUA_PB_MT);
		return 1;
	}
	case google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
			std::string str;
			str = pReflection->GetStringReference(*pMessage, pField, &str);
			lua_pushlstring(l, str.c_str(), str.size());
			return 1;
		}
	case google::protobuf::FieldDescriptor::CPPTYPE_INT32: {
			lua_pushinteger(l, pReflection->GetInt32(*pMessage, pField));
			return 1;
		}
	case google::protobuf::FieldDescriptor::CPPTYPE_UINT32: {
			lua_pushnumber(l, pReflection->GetUInt32(*pMessage, pField));
			return 1;
		}
	case google::protobuf::FieldDescriptor::CPPTYPE_INT64: {
			lua_pushnumber(l, pReflection->GetInt64(*pMessage, pField));
			return 1;
		}
	case google::protobuf::FieldDescriptor::CPPTYPE_UINT64: {
			lua_pushnumber(l, pReflection->GetUInt64(*pMessage, pField));
			return 1;
		}
	case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT: {
			lua_pushnumber(l, pReflection->GetFloat(*pMessage, pField));
			return 1;
		}
	case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE: {
			lua_pushnumber(l, pReflection->GetDouble(*pMessage, pField));
			return 1;
		}
	case google::protobuf::FieldDescriptor::CPPTYPE_BOOL: {
			lua_pushboolean(l, pReflection->GetBool(*pMessage, pField));
			return 1;
		}
	default: {
			luaL_error(l, "msg:%s, field:%s. unknow type:%d\n", pDescriptor->full_name().c_str(), pFieldName, pField->cpp_type());
			return 0;
		}
	}

	return 0;
}

static int pb_setfield(lua_State* l)
{
	luaL_argcheck(l, lua_gettop(l) == 3, 1, "expected 3 argument");
	google::protobuf::Message* pMessage = pb_tomsg(l, 1);
	pb_check(l, pMessage == NULL, "got a nil msg\n");

	const char* pFieldName = lua_tostring(l, 2);

	const google::protobuf::Descriptor* pDescriptor = pMessage->GetDescriptor();
	pb_check(l, pDescriptor == NULL, "descriptor is nil\n");

	const google::protobuf::Reflection* pReflection = pMessage->GetReflection();
	pb_check(l, pReflection == NULL, "reflection is nil\n");

	const google::protobuf::FieldDescriptor* pField = pDescriptor->FindFieldByName(pFieldName);
	pb_check(l, pField == NULL, "msg:%s, without field:%s \n", pDescriptor->full_name().c_str(), pFieldName);
	pb_check(l, pField->is_repeated(), "field is repeated %s.%s \n", pDescriptor->full_name().c_str(), pFieldName);
	pb_check(l, lua_isnil(l, 3), "value is nil!field:%s \n", pFieldName);

	switch(pField->cpp_type()) {
	case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
		if (lua_isstring(l, 3)) {
			size_t str_len = 0;
			const char* str = (const char*)lua_tolstring(l, 3, &str_len);
			google::protobuf::Message* pMessageField = pReflection->MutableMessage(pMessage, pField);
			if (pMessageField == NULL) {
				LOG("pMessageField is nil");
				return 0;
			}
			if (!pMessage->ParseFromArray(str, str_len)) {
				LOG("ParseFromArray failed\n");
				return 0;
			}
		}
		else if (lua_isuserdata(l, 3)) {
			google::protobuf::Message* pMessageField = pReflection->MutableMessage(pMessage, pField);
			if (pMessageField == NULL) {
				LOG("pMessageField is nil");
				return 0;
			}
			google::protobuf::Message* pOtherMessage = pb_tomsg(l, 3);
			if (pOtherMessage == NULL) {
				return 0;
			}
			if (pMessageField->GetDescriptor() != pOtherMessage->GetDescriptor()) {
				luaL_error(l, "type1:%s. type2:%s. type is different", pMessageField->GetTypeName().c_str(), pOtherMessage->GetTypeName().c_str());
				return 0;
			}
			pMessageField->CopyFrom(*pOtherMessage);
		}
		break;
	}
	case google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
			pb_check(l, !lua_isstring(l, 3), "value is not string!field:%s \n", pFieldName);
			size_t str_len;
			const char* str = lua_tolstring(l, 3, &str_len);
			pReflection->SetString(pMessage, pField, std::string(str, str_len));
			return 0;
		}
	case google::protobuf::FieldDescriptor::CPPTYPE_INT32: {
			pb_check(l, !lua_isinteger(l, 3), "value is not integer!field:%s \n", pFieldName);
			int32_t num = lua_tointeger(l, 3);
			pReflection->SetInt32(pMessage, pField, num);
			return 0;
		}
	case google::protobuf::FieldDescriptor::CPPTYPE_UINT32: {
			pb_check(l, !lua_isinteger(l, 3), "value is not integer!field:%s \n", pFieldName);
			uint32_t num = lua_tointeger(l, 3);
			pReflection->SetUInt32(pMessage, pField, num);
			return 0;
		}
	case google::protobuf::FieldDescriptor::CPPTYPE_INT64: {
			pb_check(l, !lua_isinteger(l, 3), "value is not integer!field:%s \n", pFieldName);
			int64_t num = lua_tointeger(l, 3);
			pReflection->SetInt64(pMessage, pField, num);
			return 0;
		}
	case google::protobuf::FieldDescriptor::CPPTYPE_UINT64: {
			pb_check(l, !lua_isinteger(l, 3), "value is not integer!field:%s \n", pFieldName);
			int64_t num = lua_tointeger(l, 3);
			pReflection->SetUInt64(pMessage, pField, num);
			return 0;
		}
	case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT: {
			pb_check(l, !lua_isnumber(l, 3), "value is not number!field:%s \n", pFieldName);
			float num = lua_tonumber(l, 3);
			pReflection->SetFloat(pMessage, pField, num);
			return 0;
		}
	case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE: {
			pb_check(l, !lua_isnumber(l, 3), "value is not number!field:%s \n", pFieldName);
			double num = lua_tonumber(l, 3);
			pReflection->SetDouble(pMessage, pField, num);
			return 0;
		}
	case google::protobuf::FieldDescriptor::CPPTYPE_BOOL: {
			pb_check(l, !lua_isboolean(l, 3), "value is not boolean!field:%s \n", pFieldName);
			int val = lua_toboolean(l, 3);
			pReflection->SetBool(pMessage, pField, val);
			return 0;
		}
	default: {
			luaL_error(l, "msg:%s, field:%s. unknow type:%d\n", pDescriptor->full_name().c_str(), pFieldName, pField->cpp_type());
			return 0;
		}
	}
	return 0;
}

static int pb_copy(lua_State* l)
{
	luaL_argcheck(l, lua_gettop(l) == 2, 1, "expected 2 argument");
	google::protobuf::Message* pMessage1 = pb_tomsg(l, 1);
	pb_check(l, pMessage1 == NULL, "got a nil msg1.\n");

	google::protobuf::Message* pMessage2 = pb_tomsg(l, 2);
	pb_check(l, pMessage2 == NULL, "got a nil msg2.\n");

	pMessage1->CopyFrom(*pMessage2);
	lua_pushboolean(l, 1);
	return 1;
}

static int pb_repeated_add(lua_State* l)
{
	luaL_argcheck(l, lua_gettop(l) >= 1, 1, "expected 1 argument at least");
	ProtoBufRepeatedField* pRepeatedField = pb_torepeated(l, 1);
	pb_check(l, pRepeatedField == NULL, "pRepeatedField is null\n");
	const google::protobuf::FieldDescriptor* pField = pRepeatedField->field;
	pb_check(l, pField == NULL, "pField is null\n");
	google::protobuf::Message* pMessage = pRepeatedField->msg;
	pb_check(l, pMessage == NULL, "pMessage is null\n");
	const google::protobuf::Reflection* pReflection = pMessage->GetReflection();
	pb_check(l, pReflection == NULL, "pReflection is null\n");

	switch (pField->cpp_type()) {
	case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
		google::protobuf::Message* pNewMessage = pReflection->AddMessage(pMessage, pField);
		pb_check(l, pNewMessage == NULL, "AddMessage failed!");
		void* p = lua_newuserdata(l, sizeof(google::protobuf::Message*));
		*(google::protobuf::Message**)p = pNewMessage;
		luaL_setmetatable(l, LUA_PB_MT);
		return 1;
	}
	case google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
		luaL_argcheck(l, lua_gettop(l) == 2, 1, "expected 2 argument at least");
		pb_check(l, !lua_isstring(l, 2), "value is not string!\n");
		size_t str_len;
		const char* str = lua_tolstring(l, 2, &str_len);
		pReflection->AddString(pMessage, pField, std::string(str, str_len));
		return 0;
	}
	case google::protobuf::FieldDescriptor::CPPTYPE_INT32: {
		luaL_argcheck(l, lua_gettop(l) == 2, 1, "expected 2 argument at least");
		pb_check(l, !lua_isinteger(l, 2), "value is not integer!\n");
		int32_t num = lua_tointeger(l, 2);
		pReflection->AddInt32(pMessage, pField, num);
		return 0;
	}
	case google::protobuf::FieldDescriptor::CPPTYPE_UINT32: {
		luaL_argcheck(l, lua_gettop(l) == 2, 1, "expected 2 argument at least");
		pb_check(l, !lua_isinteger(l, 2), "value is not integer!\n");
		uint32_t num = lua_tointeger(l, 2);
		pReflection->AddUInt32(pMessage, pField, num);
		return 0;
	}
	case google::protobuf::FieldDescriptor::CPPTYPE_INT64: {
		luaL_argcheck(l, lua_gettop(l) == 2, 1, "expected 2 argument at least");
		pb_check(l, !lua_isinteger(l, 2), "value is not integer!\n");
		int64_t num = lua_tointeger(l, 2);
		pReflection->AddInt64(pMessage, pField, num);
		return 0;
	}
	case google::protobuf::FieldDescriptor::CPPTYPE_UINT64: {
		luaL_argcheck(l, lua_gettop(l) == 2, 1, "expected 2 argument at least");
		pb_check(l, !lua_isinteger(l, 2), "value is not integer!\n");
		int64_t num = lua_tointeger(l, 2);
		pReflection->AddUInt64(pMessage, pField, num);
		return 0;
	}
	case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT: {
		luaL_argcheck(l, lua_gettop(l) == 2, 1, "expected 2 argument at least");
		pb_check(l, !lua_isnumber(l, 2), "value is not number!\n");
		float num = lua_tonumber(l, 2);
		pReflection->AddFloat(pMessage, pField, num);
		return 0;
	}
	case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE: {
		luaL_argcheck(l, lua_gettop(l) == 2, 1, "expected 2 argument at least");
		pb_check(l, !lua_isnumber(l, 2), "value is not number!field:%s \n");
		double num = lua_tonumber(l, 2);
		pReflection->AddDouble(pMessage, pField, num);
		return 0;
	}
	case google::protobuf::FieldDescriptor::CPPTYPE_BOOL: {
		luaL_argcheck(l, lua_gettop(l) == 2, 1, "expected 2 argument at least");
		pb_check(l, !lua_isboolean(l, 2), "value is not boolean!\n");
		int val = lua_toboolean(l, 2);
		pReflection->AddBool(pMessage, pField, val);
		return 0;
	}
	default: {
			luaL_error(l, "msg:%s, field:%s. unknow type:%d\n", pMessage->GetDescriptor()->full_name().c_str(), pField->full_name(), pField->cpp_type());
			return 0;
		}
	}

	return 0;
}

static int pb_repeated_field(lua_State* l)
{
	ProtoBufRepeatedField* pRepeatedField = pb_torepeated(l, 1);
	pb_check(l, pRepeatedField == NULL, "pRepeatedField is null\n");

	int idx = lua_tointeger(l, 2);
	pb_check(l, idx <= 0, "index must be greater than 0\n");
	idx = idx - 1;

	const google::protobuf::FieldDescriptor* pField = pRepeatedField->field;
	pb_check(l, pField == NULL, "pField is null\n");
	google::protobuf::Message* pMessage = pRepeatedField->msg;
	pb_check(l, pMessage == NULL, "pMessage is null\n");

	const google::protobuf::Reflection* pReflection = pMessage->GetReflection();
	pb_check(l, pReflection == NULL, "pReflection is null\n");

	if (pReflection->FieldSize(*pMessage, pField) <= idx) {
		lua_pushnil(l);
		return 1;
	}

	switch (pField->cpp_type()) {
		case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
			google::protobuf::Message* pNewMessage = pReflection->MutableRepeatedMessage(pMessage, pField, idx);
			if (pNewMessage == NULL) {
				LOG("pNewMessage is nil\n");
				return 0;
			}
			void* p = lua_newuserdata(l, sizeof(google::protobuf::Message*));
			*(google::protobuf::Message**)p = pNewMessage;
			luaL_setmetatable(l, LUA_PB_MT);
			return 1;
		}
		case google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
			std::string str;
			str = pReflection->GetRepeatedStringReference(*pMessage, pField, idx, &str);
			lua_pushlstring(l, str.c_str(), str.size());
			return 1;
		}
		case google::protobuf::FieldDescriptor::CPPTYPE_INT32: {
			lua_pushinteger(l, pReflection->GetRepeatedInt32(*pMessage, pField, idx));
			return 1;
		}
		case google::protobuf::FieldDescriptor::CPPTYPE_UINT32: {
			lua_pushinteger(l, pReflection->GetRepeatedUInt32(*pMessage, pField, idx));
			return 1;
		}
		case google::protobuf::FieldDescriptor::CPPTYPE_INT64: {
			lua_pushinteger(l, pReflection->GetRepeatedInt64(*pMessage, pField, idx));
			return 1;
		}
		case google::protobuf::FieldDescriptor::CPPTYPE_UINT64: {
			lua_pushinteger(l, pReflection->GetRepeatedUInt64(*pMessage, pField, idx));
			return 1;
		}
		case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT: {
			lua_pushnumber(l, pReflection->GetRepeatedFloat(*pMessage, pField, idx));
			return 1;
		}
		case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE: {
			lua_pushnumber(l, pReflection->GetRepeatedDouble(*pMessage, pField, idx));
			return 1;
		}
		case google::protobuf::FieldDescriptor::CPPTYPE_BOOL: {
			lua_pushnumber(l, pReflection->GetRepeatedBool(*pMessage, pField, idx));
			return 1;
		}
		default: {
				luaL_error(l, "msg:%s, field:%s. unknow type:%d\n", pMessage->GetDescriptor()->full_name().c_str(), pField->full_name(), pField->cpp_type());
				return 0;
		}
	}
}

static int pb_repeated_size(lua_State* l)
{
	ProtoBufRepeatedField* pRepeatedField = pb_torepeated(l, 1);
	pb_check(l, pRepeatedField == NULL, "pRepeatedField is null\n");

	const google::protobuf::FieldDescriptor* pField = pRepeatedField->field;
	pb_check(l, pField == NULL, "pField is null\n");
	google::protobuf::Message* pMessage = pRepeatedField->msg;
	pb_check(l, pMessage == NULL, "pMessage is null\n");

	const google::protobuf::Reflection* pReflection = pMessage->GetReflection();
	pb_check(l, pReflection == NULL, "pReflection is null\n");

	lua_pushinteger(l, pReflection->FieldSize(*pMessage, pField));
	return 1;
}

static int pb_repeated_copy(lua_State* l)
{
	luaL_argcheck(l, lua_gettop(l) == 2, 1, "expected 2 arguments");
	ProtoBufRepeatedField* pRepeatedField1 = pb_torepeated(l, 1);
	pb_check(l, pRepeatedField1 == NULL, "pRepeatedField2 is null\n");

	const google::protobuf::FieldDescriptor* pField1 = pRepeatedField1->field;
	pb_check(l, pField1 == NULL, "pField2 is null\n");
	google::protobuf::Message* pMessage1 = pRepeatedField1->msg;
	pb_check(l, pMessage1 == NULL, "pMessage1 is null\n");

	const google::protobuf::Reflection* pReflection1 = pMessage1->GetReflection();
	pb_check(l, pReflection1 == NULL, "pReflection1 is null\n");


	ProtoBufRepeatedField* pRepeatedField2 = pb_torepeated(l, 2);
	pb_check(l, pRepeatedField2 == NULL, "pRepeatedField1 is null\n");

	const google::protobuf::FieldDescriptor* pField2 = pRepeatedField2->field;
	pb_check(l, pField2 == NULL, "pField2 is null\n");
	google::protobuf::Message* pMessage2 = pRepeatedField2->msg;
	pb_check(l, pMessage2 == NULL, "pMessage2 is null\n");

	const google::protobuf::Reflection* pReflection2 = pMessage2->GetReflection();
	pb_check(l, pReflection2 == NULL, "pReflection2 is null\n");

	pb_check(l, pField1->cpp_type() != pField2->cpp_type(), "type not same.msg1:%s, msg2:%s\n", pField1->full_name().c_str(), pField2->full_name().c_str());

	int sz = pReflection2->FieldSize(*pMessage2, pField2);
	for (int i = 0; i < sz; ++i) {
		if (pField1->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_STRING)
		{
			pReflection1->AddString(pMessage1, pField1, pReflection2->GetRepeatedString(*pMessage2, pField2, i));
		}
		else if (pField1->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_INT32)
		{
			pReflection1->AddInt32(pMessage1, pField1, pReflection2->GetRepeatedInt32(*pMessage2, pField2, i));
		}
		else if (pField1->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_UINT32)
		{
			pReflection1->AddUInt32(pMessage1, pField1, pReflection2->GetRepeatedUInt32(*pMessage2, pField2, i));
		}
		else if (pField1->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_INT64)
		{
			pReflection1->AddInt64(pMessage1, pField1, pReflection2->GetRepeatedInt64(*pMessage2, pField2, i));
		}
		else if (pField1->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_UINT64)
		{
			pReflection1->AddUInt64(pMessage1, pField1, pReflection2->GetRepeatedUInt64(*pMessage2, pField2, i));
		}
		else if (pField1->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_FLOAT)
		{
			pReflection1->AddFloat(pMessage1, pField1, pReflection2->GetRepeatedFloat(*pMessage2, pField2, i));
		}
		else if (pField1->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE)
		{
			pReflection1->AddDouble(pMessage1, pField1, pReflection2->GetRepeatedDouble(*pMessage2, pField2, i));
		}
		else if (pField1->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_BOOL)
		{
			pReflection1->AddBool(pMessage1, pField1, pReflection2->GetRepeatedBool(*pMessage2, pField2, i));
		}
		else if (pField1->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
		{
			google::protobuf::Message* pSubMessage1 = pReflection1->AddMessage(pMessage1, pField1);
			const google::protobuf::Message& pSubMessage2 = pReflection2->GetRepeatedMessage(*pMessage2, pField2, i);
			if (pSubMessage1 == NULL)
			{
				pReflection1->ClearField(pMessage1, pField1);
				LOG("add mesage null\n");
				return 0;
			}
			pSubMessage1->CopyFrom(pSubMessage2);
		}
		else
		{
			LOG("add fail file type :%d", pField1->cpp_type());
		}
	}

	lua_pushboolean(l, 1);
	return 1;
}

static int pb_repeated_tostring(lua_State* l)
{
	luaL_argcheck(l, lua_gettop(l) == 1, 1, "expected 1 arguments");
	ProtoBufRepeatedField* pRepeatedField = pb_torepeated(l, 1);
	pb_check(l, pRepeatedField == NULL, "pRepeatedField is null\n");

	const google::protobuf::FieldDescriptor* pField = pRepeatedField->field;
	pb_check(l, pField == NULL, "pField is null\n");
	google::protobuf::Message* pMessage = pRepeatedField->msg;
	pb_check(l, pMessage == NULL, "pMessage is null\n");

	const google::protobuf::Reflection* pReflection = pMessage->GetReflection();
	pb_check(l, pReflection == NULL, "pReflection is null\n");

	std::string str = pField->DebugString();
	lua_pushlstring(l, str.c_str(), str.size());
	return 1;
}

static int pb_repeated_find_message(lua_State* l)
{
	luaL_argcheck(l, lua_gettop(l) == 3, 1, "expected 3 arguments");
	ProtoBufRepeatedField* pRepeatedField = pb_torepeated(l, 1);
	pb_check(l, pRepeatedField == NULL, "pRepeatedField is null\n");

	const google::protobuf::FieldDescriptor* pField = pRepeatedField->field;
	pb_check(l, pField == NULL, "pField is null\n");

	google::protobuf::Message* pMessage = pRepeatedField->msg;
	pb_check(l, pMessage == NULL, "pMessage is null\n");

	const google::protobuf::Reflection* pReflection = pMessage->GetReflection();
	pb_check(l, pReflection == NULL, "pReflection is null\n");

	const char* pFindFieldName = lua_tostring(l, 2);
	pb_check(l, pFindFieldName == NULL, "find field is null\n");

	const google::protobuf::Descriptor* pSubDescriptor = pField->message_type();
	pb_check(l, pSubDescriptor == NULL, "pSubDescriptor is null\n");
	const google::protobuf::FieldDescriptor* pSubField = pSubDescriptor->FindFieldByName(pFindFieldName);
	pb_check(l, pSubField == NULL, "pSubField is null\n");

	int sz = pReflection->FieldSize(*pMessage, pField);
	int idx = -1;

#define FINDKEY_BEGIN \
	for (int i = 0; i < sz; ++i) { \
		const google::protobuf::Message& pSubFieldMessage = pReflection->GetRepeatedMessage(*pMessage, pField, i); \
		const google::protobuf::Reflection* pSubFieldReflection = pSubFieldMessage.GetReflection();
#define FINDKEY_CHECK(exp) if(exp){ idx = i; break; }
#define FINDKEY_END }

	switch (pSubField->cpp_type()) {
		case google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
			const char* val = lua_tostring(l, 3);
			FINDKEY_BEGIN
				std::string str;
				str = pSubFieldReflection->GetStringReference(pSubFieldMessage, pSubField, &str);
				FINDKEY_CHECK(!strncmp(val, str.c_str(), str.size()));
			FINDKEY_END
		}break;
		case google::protobuf::FieldDescriptor::CPPTYPE_INT32: {
			int32_t val = lua_tointeger(l, 3);
			FINDKEY_BEGIN
				FINDKEY_CHECK(pSubFieldReflection->GetInt32(pSubFieldMessage, pSubField) == val)
			FINDKEY_END
		}break;
		case google::protobuf::FieldDescriptor::CPPTYPE_UINT32: {
			uint32_t val = lua_tointeger(l, 3);
			FINDKEY_BEGIN
				FINDKEY_CHECK(pSubFieldReflection->GetUInt32(pSubFieldMessage, pSubField) == val)
			FINDKEY_END
		}break;
		case google::protobuf::FieldDescriptor::CPPTYPE_INT64: {
			int64_t val = lua_tointeger(l, 3);
			FINDKEY_BEGIN
				FINDKEY_CHECK(pSubFieldReflection->GetInt64(pSubFieldMessage, pSubField) == val)
			FINDKEY_END
		}break;
		case google::protobuf::FieldDescriptor::CPPTYPE_UINT64: {
			uint64_t val = lua_tointeger(l, 3);
			FINDKEY_BEGIN
				FINDKEY_CHECK(pSubFieldReflection->GetUInt64(pSubFieldMessage, pSubField) == val)
			FINDKEY_END
		}break;
		case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT: {
			float val = lua_tonumber(l, 3);
			FINDKEY_BEGIN
				FINDKEY_CHECK(pSubFieldReflection->GetFloat(pSubFieldMessage, pSubField) == val)
			FINDKEY_END
		}break;
		case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE: {
			double val = lua_tonumber(l, 3);
			FINDKEY_BEGIN
				FINDKEY_CHECK(pSubFieldReflection->GetDouble(pSubFieldMessage, pSubField) == val)
			FINDKEY_END
		}break;
		case google::protobuf::FieldDescriptor::CPPTYPE_BOOL: {
			int val = lua_toboolean(l, 3);
			FINDKEY_BEGIN
				FINDKEY_CHECK(int(pSubFieldReflection->GetBool(pSubFieldMessage, pSubField)) == val)
			FINDKEY_END
		}break;
		default: {
			luaL_error(l, "msg:%s, field:%s. unknow type:%d\n", pSubDescriptor->full_name().c_str(), pSubField->full_name(), pSubField->cpp_type());
			return 0;
		}
	}

	if (idx < 0) {
		return 0;
	}

	lua_pushinteger(l, idx + 1);

	google::protobuf::Message* pFindMessage = pReflection->MutableRepeatedMessage(pMessage, pField, idx);
	pb_check(l, pFindMessage == NULL, "pFindMessage is null\n");
	void* p = lua_newuserdata(l, sizeof(google::protobuf::Message*));
	*(google::protobuf::Message**)p = pFindMessage;
	luaL_setmetatable(l, LUA_PB_MT);
	return 2;
}

static int pb_repeated_find_othertype(lua_State* l)
{
	luaL_argcheck(l, lua_gettop(l) == 2, 1, "expected 2 arguments");
	ProtoBufRepeatedField* pRepeatedField = pb_torepeated(l, 1);
	pb_check(l, pRepeatedField == NULL, "pRepeatedField is null\n");

	const google::protobuf::FieldDescriptor* pField = pRepeatedField->field;
	pb_check(l, pField == NULL, "pField is null\n");

	google::protobuf::Message* pMessage = pRepeatedField->msg;
	pb_check(l, pMessage == NULL, "pMessage is null\n");

	const google::protobuf::Reflection* pReflection = pMessage->GetReflection();
	pb_check(l, pReflection == NULL, "pReflection is null\n");

	int sz = pReflection->FieldSize(*pMessage, pField);
	int idx = -1;

#define FINDKEY_OTHER_BEGIN \
	for (int i = 0; i < sz; ++i) { 
#define FINDKEY_OTHER_CHECK(exp) if(exp) { idx = i; break; }
#define FINDKEY_OTHER_END }
	switch (pField->cpp_type()) {
	case google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
		const char* val = lua_tostring(l, 2);
		FINDKEY_OTHER_BEGIN
			std::string str = pReflection->GetRepeatedString(*pMessage, pField, i);
			FINDKEY_OTHER_CHECK(!strncmp(val, str.c_str(), str.size()))
		FINDKEY_OTHER_END
	}break;
	case google::protobuf::FieldDescriptor::CPPTYPE_INT32: {
		int32_t val = lua_tointeger(l, 2);
		FINDKEY_OTHER_BEGIN
			FINDKEY_OTHER_CHECK(pReflection->GetRepeatedInt32(*pMessage, pField, i) == val)
		FINDKEY_OTHER_END
	}break;
	case google::protobuf::FieldDescriptor::CPPTYPE_UINT32: {
		uint32_t val = lua_tointeger(l, 2);
		FINDKEY_OTHER_BEGIN
			FINDKEY_OTHER_CHECK(pReflection->GetRepeatedUInt32(*pMessage, pField, i) == val)
		FINDKEY_OTHER_END
	}break;
	case google::protobuf::FieldDescriptor::CPPTYPE_INT64: {
		int64_t val = lua_tointeger(l, 2);
		FINDKEY_OTHER_BEGIN
			FINDKEY_OTHER_CHECK(pReflection->GetRepeatedInt64(*pMessage, pField, i) == val)
		FINDKEY_OTHER_END
	}break;
	case google::protobuf::FieldDescriptor::CPPTYPE_UINT64: {
		uint64_t val = lua_tointeger(l, 2);
		FINDKEY_OTHER_BEGIN
			FINDKEY_OTHER_CHECK(pReflection->GetRepeatedUInt64(*pMessage, pField, i) == val)
		FINDKEY_OTHER_END
	}break;
	case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT: {
		float val = lua_tonumber(l, 2);
		FINDKEY_OTHER_BEGIN
			FINDKEY_OTHER_CHECK(pReflection->GetRepeatedFloat(*pMessage, pField, i) == val)
		FINDKEY_OTHER_END
	}break;
	case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE: {
		double val = lua_tonumber(l, 2);
		FINDKEY_OTHER_BEGIN
			FINDKEY_OTHER_CHECK(pReflection->GetRepeatedDouble(*pMessage, pField, i) == val)
		FINDKEY_OTHER_END
	}break;
	case google::protobuf::FieldDescriptor::CPPTYPE_BOOL: {
		int val = lua_toboolean(l, 2);
		FINDKEY_OTHER_BEGIN
			FINDKEY_OTHER_CHECK(int(pReflection->GetRepeatedBool(*pMessage, pField, i)) == val)
		FINDKEY_OTHER_END
	}break;
	default: {
			luaL_error(l, "field:%s. unknow type:%d\n", pField->full_name(), pField->cpp_type());
			return 0;
		}
	}

	if (idx < 0) {
		return 0;
	}

	lua_pushinteger(l, idx + 1);
	return 1;
}

static int pb_repeated_find(lua_State* l)
{
	luaL_argcheck(l, lua_gettop(l) >= 2, 1, "expected 2 arguments");

	ProtoBufRepeatedField* pRepeatedField = pb_torepeated(l, 1);
	pb_check(l, pRepeatedField == NULL, "pRepeatedField is null\n");

	const google::protobuf::FieldDescriptor* pField = pRepeatedField->field;
	pb_check(l, pField == NULL, "pField is null\n");

	if (pField->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
		return pb_repeated_find_message(l);
	}else{
		return pb_repeated_find_othertype(l);
	}

	return 0;
}

static const luaL_Reg pblib[] = {
	{ "encode", pb_encode },
	{ "decode", pb_decode },
	{ "tostring", pb_tostring },
	{ "copy", pb_copy },
	{NULL, NULL}
};

static int pb_metafunc(lua_State* l)
{
	luaL_argcheck(l, lua_gettop(l) == 2, 1, "expected 2 argument");
	const char* pFieldName = luaL_checkstring(l, 2);
	int sz = (sizeof(pblib) / sizeof(luaL_Reg)) - 1;
	for (int i = 0; i < sz; ++i) {
		if (!strcmp(pblib[i].name, pFieldName)) {
			lua_pushcfunction(l, pblib[i].func);
			return 1;
		}
	}

	return pb_field(l);
}

static const luaL_Reg pb_repeated_lib[] = {
	{ "add", pb_repeated_add },
	{ "copy", pb_repeated_copy },
	{ "find", pb_repeated_find },
	{NULL, NULL}
};

static int pb_repeated_metafunc(lua_State* l)
{
	luaL_argcheck(l, lua_gettop(l) == 2, 1, "expected 2 argument");
	int type = lua_type(l, 2);
	if (type == LUA_TSTRING) {
		const char* pFieldName = luaL_checkstring(l, 2);
		int sz = (sizeof(pb_repeated_lib) / sizeof(luaL_Reg)) - 1;
		for (int i = 0; i < sz; ++i) {
			if (!strcmp(pb_repeated_lib[i].name, pFieldName)) {
				lua_pushcfunction(l, pb_repeated_lib[i].func);
				return 1;
			}
		}
	}
	else {
		if (type == LUA_TNUMBER) {
			return pb_repeated_field(l);
		}
	}

	return 0;
}

int init_pb(lua_State* l)
{
	luaL_newmetatable(l, LUA_PB_MT);
	lua_pushcfunction(l, pb_metafunc);
	lua_setfield(l, -2, "__index");

	lua_pushcfunction(l, pb_setfield);
	lua_setfield(l, -2, "__newindex");

	lua_pushcfunction(l, pb_tostring);
	lua_setfield(l, -2, "__tostring");

	lua_pop(l, 1);

	luaL_newmetatable(l, LUA_PB_REPEATED_MT);
	lua_pushcfunction(l, pb_repeated_metafunc);
	lua_setfield(l, -2, "__index");

	lua_pushcfunction(l, pb_repeated_size);
	lua_setfield(l, -2, "__len");

	lua_pushcfunction(l, pb_repeated_tostring);
	lua_setfield(l, -2, "__tostring");

	lua_pop(l, 1);

	return 0;
}