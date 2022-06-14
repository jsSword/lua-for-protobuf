#ifndef __PB_LOADER_H__
#define __PB_LOADER_H__
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/compiler/importer.h>
#include <string>

class ProtoBuffLoader
{
private:
	ProtoBuffLoader();
public:
	~ProtoBuffLoader();

	void MapPath(const std::string& pcVirtualPath, const std::string& pcDiskPath);
	int ImportFile(const std::string& pcFileName);
	google::protobuf::Message* CreateMessage(const std::string& pcMsgName);
	google::protobuf::Message* CreateDynMessage(const std::string& pcMsgName);
	google::protobuf::Message* CreateStaticMessage(const std::string& pcMsgName);
	std::string GetMsgFullName(google::protobuf::Message* pMsg);
	const google::protobuf::Descriptor* GetDescriptorByName(const std::string& pcMsgName);
	const google::protobuf::Message* GetProtoMessageByName(const std::string& pcMsgName);

	void DestroyAll();
public:
	static ProtoBuffLoader* getInstance() 
	{
		static ProtoBuffLoader pbloader;
		return &pbloader;
	}
private:
	google::protobuf::compiler::DiskSourceTree* m_pSourceTree;
	google::protobuf::compiler::SourceTreeDescriptorDatabase* m_pDataBase;
	google::protobuf::DescriptorPool* m_pDescPool;
	google::protobuf::DynamicMessageFactory* m_pMsgFactory;
};

#define g_pProtoLoader ProtoBuffLoader::getInstance()
#endif // !__PB_LOADER_H__

