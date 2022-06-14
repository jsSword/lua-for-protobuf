#include "pbloader.h"

class MyMultiFileErrorCollector : public google::protobuf::compiler::MultiFileErrorCollector {
	virtual void AddError(
		const std::string& filename,
		int line,
		int column,
		const std::string& message) {
		fprintf(stderr, "%s,%d,%d,%s\n", filename.c_str(), line, column, message.c_str());
	}
};

static MyMultiFileErrorCollector g_pErrorCtrl;

ProtoBuffLoader::ProtoBuffLoader()
{
	m_pSourceTree = new google::protobuf::compiler::DiskSourceTree();
	m_pDataBase = new google::protobuf::compiler::SourceTreeDescriptorDatabase(m_pSourceTree);
	m_pDataBase->RecordErrorsTo(&g_pErrorCtrl);

	m_pDescPool = new google::protobuf::DescriptorPool(m_pDescPool->generated_pool());

	m_pMsgFactory = new google::protobuf::DynamicMessageFactory(m_pDescPool);
	m_pMsgFactory->SetDelegateToGeneratedFactory(true);
}

ProtoBuffLoader::~ProtoBuffLoader()
{
	delete m_pMsgFactory;
	delete m_pDescPool;
	delete m_pDataBase;
	delete m_pSourceTree;
}

void ProtoBuffLoader::MapPath(const std::string& pcVirtualPath, const std::string& pcDiskPath)
{
	m_pSourceTree->MapPath(pcVirtualPath, pcDiskPath);
}

int ProtoBuffLoader::ImportFile(const std::string& pcFileName)
{
	google::protobuf::FileDescriptorProto stFileDesc;
	if (!m_pDataBase->FindFileByName(pcFileName, &stFileDesc))
	{
		return -100;
	}

	if (!m_pDescPool->BuildFile(stFileDesc))
	{
		return -200;
	}

	return 0;
}

google::protobuf::Message* ProtoBuffLoader::CreateMessage(const std::string& pcMsgName)
{
	return CreateDynMessage(pcMsgName);
}

google::protobuf::Message* ProtoBuffLoader::CreateDynMessage(const std::string& pcMsgName)
{
	const google::protobuf::Descriptor* pDescriptor = m_pDescPool->FindMessageTypeByName(pcMsgName);
	if (!pDescriptor)
	{
		return NULL;
	}

	const google::protobuf::Message* pMessage = m_pMsgFactory->GetPrototype(pDescriptor);
	if (!pMessage)
	{
		return NULL;
	}

	google::protobuf::Message* pNewMsg = pMessage->New();

	return pNewMsg;
}

google::protobuf::Message* ProtoBuffLoader::CreateStaticMessage(const std::string& pcMsgName)
{
	const google::protobuf::Descriptor* pDescriptor = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(pcMsgName);
	if (!pDescriptor)
	{
		return NULL;
	}

	const google::protobuf::Message* pMessage = google::protobuf::MessageFactory::generated_factory()->GetPrototype(pDescriptor);
	if (!pMessage)
	{
		return NULL;
	}

	google::protobuf::Message* pNewMsg = pMessage->New();

	return pNewMsg;
}

std::string ProtoBuffLoader::GetMsgFullName(google::protobuf::Message* pMsg)
{
	const google::protobuf::Descriptor* pDescriptor = pMsg->GetDescriptor();

	if (!pDescriptor)
	{
		return "";
	}

	return pDescriptor->full_name();
}

const google::protobuf::Descriptor* ProtoBuffLoader::GetDescriptorByName(const std::string& pcMsgName)
{
	return m_pDescPool->FindMessageTypeByName(pcMsgName);
}

void ProtoBuffLoader::DestroyAll()
{
	google::protobuf::ShutdownProtobufLibrary();
}

const google::protobuf::Message* ProtoBuffLoader::GetProtoMessageByName(const std::string& pcMsgName)
{
	const google::protobuf::Descriptor* pDescriptor = m_pDescPool->FindMessageTypeByName(pcMsgName);
	if (!pDescriptor)
	{
		return NULL;
	}

	const google::protobuf::Message* pMessage = m_pMsgFactory->GetPrototype(pDescriptor);

	return pMessage;
}
