#include <Foundation/FoundationPCH.h>

#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/GraphVersioning.h>

enum xiiBinarySerializerVersion : xiiUInt32
{
  InvalidVersion = 0,
  Version1,
  // << insert new versions here >>

  ENUM_COUNT,
  CurrentVersion = ENUM_COUNT - 1 // automatically the highest version number
};

static void WriteGraph(const xiiAbstractObjectGraph* pGraph, xiiStreamWriter& stream)
{
  const auto& Nodes = pGraph->GetAllNodes();

  xiiUInt32 uiNodes = Nodes.GetCount();
  stream << uiNodes;
  for (auto itNode = Nodes.GetIterator(); itNode.IsValid(); ++itNode)
  {
    const auto& node = *itNode.Value();
    stream << node.GetGuid();
    stream << node.GetType();
    stream << node.GetTypeVersion();
    stream << node.GetNodeName();

    const xiiHybridArray<xiiAbstractObjectNode::Property, 16>& properties = node.GetProperties();
    xiiUInt32                                                  uiProps    = properties.GetCount();
    stream << uiProps;
    for (const xiiAbstractObjectNode::Property& prop : properties)
    {
      stream << prop.m_szPropertyName;
      stream << prop.m_Value;
    }
  }
}

void xiiAbstractGraphBinarySerializer::Write(xiiStreamWriter& stream, const xiiAbstractObjectGraph* pGraph, const xiiAbstractObjectGraph* pTypesGraph)
{
  xiiUInt32 uiVersion = xiiBinarySerializerVersion::CurrentVersion;
  stream << uiVersion;

  WriteGraph(pGraph, stream);
  if (pTypesGraph)
  {
    WriteGraph(pTypesGraph, stream);
  }
}

static void ReadGraph(xiiStreamReader& stream, xiiAbstractObjectGraph* pGraph)
{
  xiiUInt32 uiNodes = 0;
  stream >> uiNodes;
  for (xiiUInt32 uiNodeIdx = 0; uiNodeIdx < uiNodes; uiNodeIdx++)
  {
    xiiUuid          guid;
    xiiUInt32        uiTypeVersion;
    xiiStringBuilder sType;
    xiiStringBuilder sNodeName;
    stream >> guid;
    stream >> sType;
    stream >> uiTypeVersion;
    stream >> sNodeName;
    xiiAbstractObjectNode* pNode   = pGraph->AddNode(guid, sType, uiTypeVersion, sNodeName);
    xiiUInt32              uiProps = 0;
    stream >> uiProps;
    for (xiiUInt32 propIdx = 0; propIdx < uiProps; ++propIdx)
    {
      xiiStringBuilder sPropName;
      xiiVariant       value;
      stream >> sPropName;
      stream >> value;
      pNode->AddProperty(sPropName, value);
    }
  }
}

void xiiAbstractGraphBinarySerializer::Read(
  xiiStreamReader&        stream,
  xiiAbstractObjectGraph* pGraph,
  xiiAbstractObjectGraph* pTypesGraph,
  bool                    bApplyPatches)
{
  xiiUInt32 uiVersion = 0;
  stream >> uiVersion;
  if (uiVersion != xiiBinarySerializerVersion::CurrentVersion)
  {
    XII_REPORT_FAILURE(
      "Binary serializer version {0} does not match expected version {1}, re-export file.", uiVersion, xiiBinarySerializerVersion::CurrentVersion);
    return;
  }
  ReadGraph(stream, pGraph);
  if (pTypesGraph)
  {
    ReadGraph(stream, pTypesGraph);
  }

  if (bApplyPatches)
  {
    if (pTypesGraph)
      xiiGraphVersioning::GetSingleton()->PatchGraph(pTypesGraph);
    xiiGraphVersioning::GetSingleton()->PatchGraph(pGraph, pTypesGraph);
  }
}

XII_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_BinarySerializer);
