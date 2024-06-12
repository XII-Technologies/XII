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

static void WriteGraph(const xiiAbstractObjectGraph* pGraph, xiiStreamWriter& ref_stream)
{
  const auto& Nodes = pGraph->GetAllNodes();

  xiiUInt32 uiNodes = Nodes.GetCount();
  ref_stream << uiNodes;
  for (auto itNode = Nodes.GetIterator(); itNode.IsValid(); ++itNode)
  {
    const auto& node = *itNode.Value();
    ref_stream << node.GetGuid();
    ref_stream << node.GetType();
    ref_stream << node.GetTypeVersion();
    ref_stream << node.GetNodeName();

    const xiiHybridArray<xiiAbstractObjectNode::Property, 16>& properties = node.GetProperties();
    xiiUInt32                                                  uiProps    = properties.GetCount();
    ref_stream << uiProps;
    for (const xiiAbstractObjectNode::Property& prop : properties)
    {
      ref_stream << prop.m_sPropertyName;
      ref_stream << prop.m_Value;
    }
  }
}

void xiiAbstractGraphBinarySerializer::Write(xiiStreamWriter& ref_stream, const xiiAbstractObjectGraph* pGraph, const xiiAbstractObjectGraph* pTypesGraph)
{
  xiiUInt32 uiVersion = xiiBinarySerializerVersion::CurrentVersion;
  ref_stream << uiVersion;

  WriteGraph(pGraph, ref_stream);
  if (pTypesGraph)
  {
    WriteGraph(pTypesGraph, ref_stream);
  }
}

static void ReadGraph(xiiStreamReader& ref_stream, xiiAbstractObjectGraph* pGraph)
{
  xiiUInt32 uiNodes = 0;
  ref_stream >> uiNodes;
  for (xiiUInt32 uiNodeIdx = 0; uiNodeIdx < uiNodes; uiNodeIdx++)
  {
    xiiUuid          guid;
    xiiUInt32        uiTypeVersion;
    xiiStringBuilder sType;
    xiiStringBuilder sNodeName;
    ref_stream >> guid;
    ref_stream >> sType;
    ref_stream >> uiTypeVersion;
    ref_stream >> sNodeName;
    xiiAbstractObjectNode* pNode   = pGraph->AddNode(guid, sType, uiTypeVersion, sNodeName);
    xiiUInt32              uiProps = 0;
    ref_stream >> uiProps;
    for (xiiUInt32 propIdx = 0; propIdx < uiProps; ++propIdx)
    {
      xiiStringBuilder sPropName;
      xiiVariant       value;
      ref_stream >> sPropName;
      ref_stream >> value;
      pNode->AddProperty(sPropName, value);
    }
  }
}

void xiiAbstractGraphBinarySerializer::Read(xiiStreamReader& ref_stream, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectGraph* pTypesGraph, bool bApplyPatches)
{
  xiiUInt32 uiVersion = 0;
  ref_stream >> uiVersion;
  if (uiVersion != xiiBinarySerializerVersion::CurrentVersion)
  {
    XII_REPORT_FAILURE("Binary serializer version {0} does not match expected version {1}, re-export file.", uiVersion, xiiBinarySerializerVersion::CurrentVersion);
    return;
  }
  ReadGraph(ref_stream, pGraph);
  if (pTypesGraph)
  {
    ReadGraph(ref_stream, pTypesGraph);
  }

  if (bApplyPatches)
  {
    if (pTypesGraph)
      xiiGraphVersioning::GetSingleton()->PatchGraph(pTypesGraph);
    xiiGraphVersioning::GetSingleton()->PatchGraph(pGraph, pTypesGraph);
  }
}

XII_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_BinarySerializer);
