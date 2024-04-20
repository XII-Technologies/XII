#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/GraphVersioning.h>

namespace
{
  xiiSerializedBlock* FindBlock(xiiHybridArray<xiiSerializedBlock, 3>& ref_blocks, xiiStringView sName)
  {
    for (auto& block : ref_blocks)
    {
      if (block.m_Name == sName)
      {
        return &block;
      }
    }
    return nullptr;
  }

  xiiSerializedBlock* FindHeaderBlock(xiiHybridArray<xiiSerializedBlock, 3>& ref_blocks, xiiInt32& out_iVersion)
  {
    xiiStringBuilder sHeaderName = "HeaderV";
    out_iVersion                 = 0;
    for (auto& block : ref_blocks)
    {
      if (block.m_Name.StartsWith(sHeaderName))
      {
        xiiResult res = xiiConversionUtils::StringToInt(block.m_Name.GetData() + sHeaderName.GetElementCount(), out_iVersion);
        if (res.Failed())
        {
          xiiLog::Error("Failed to parse version from header name '{0}'", block.m_Name);
        }
        return &block;
      }
    }
    return nullptr;
  }

  xiiSerializedBlock* GetOrCreateBlock(xiiHybridArray<xiiSerializedBlock, 3>& ref_blocks, xiiStringView sName)
  {
    xiiSerializedBlock* pBlock = FindBlock(ref_blocks, sName);
    if (!pBlock)
    {
      pBlock         = &ref_blocks.ExpandAndGetRef();
      pBlock->m_Name = sName;
    }
    if (!pBlock->m_Graph)
    {
      pBlock->m_Graph = XII_DEFAULT_NEW(xiiAbstractObjectGraph);
    }
    return pBlock;
  }
} // namespace

static void WriteGraph(xiiOpenDdlWriter& ref_writer, const xiiAbstractObjectGraph* pGraph, xiiStringView sName)
{
  xiiMap<xiiStringView, const xiiVariant*> SortedProperties;

  ref_writer.BeginObject(sName);

  const auto& Nodes = pGraph->GetAllNodes();
  for (auto itNode = Nodes.GetIterator(); itNode.IsValid(); ++itNode)
  {
    const auto& node = *itNode.Value();

    ref_writer.BeginObject("o");

    {
      xiiOpenDdlUtils::StoreUuid(ref_writer, node.GetGuid(), "id");
      xiiOpenDdlUtils::StoreString(ref_writer, node.GetType(), "t");
      xiiOpenDdlUtils::StoreUInt32(ref_writer, node.GetTypeVersion(), "v");

      if (!node.GetNodeName().IsEmpty())
        xiiOpenDdlUtils::StoreString(ref_writer, node.GetNodeName(), "n");

      ref_writer.BeginObject("p");
      {
        for (const auto& prop : node.GetProperties())
          SortedProperties[prop.m_sPropertyName] = &prop.m_Value;

        for (auto it = SortedProperties.GetIterator(); it.IsValid(); ++it)
        {
          xiiOpenDdlUtils::StoreVariant(ref_writer, *it.Value(), it.Key());
        }

        SortedProperties.Clear();
      }
      ref_writer.EndObject();
    }
    ref_writer.EndObject();
  }

  ref_writer.EndObject();
}

void xiiAbstractGraphDdlSerializer::Write(xiiStreamWriter& ref_stream, const xiiAbstractObjectGraph* pGraph, const xiiAbstractObjectGraph* pTypesGraph, bool bCompactMmode, xiiOpenDdlWriter::TypeStringMode typeMode)
{
  xiiOpenDdlWriter writer;
  writer.SetOutputStream(&ref_stream);
  writer.SetCompactMode(bCompactMmode);
  writer.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Exact);
  writer.SetPrimitiveTypeStringMode(typeMode);

  if (typeMode != xiiOpenDdlWriter::TypeStringMode::Compliant)
    writer.SetIndentation(-1);

  Write(writer, pGraph, pTypesGraph);
}

void xiiAbstractGraphDdlSerializer::Write(xiiOpenDdlWriter& ref_writer, const xiiAbstractObjectGraph* pGraph, const xiiAbstractObjectGraph* pTypesGraph /*= nullptr*/)
{
  WriteGraph(ref_writer, pGraph, "Objects");
  if (pTypesGraph)
  {
    WriteGraph(ref_writer, pTypesGraph, "Types");
  }
}

static void ReadGraph(xiiAbstractObjectGraph* pGraph, const xiiOpenDdlReaderElement* pRoot)
{
  xiiStringBuilder tmp, tmp2;
  xiiVariant       varTmp;

  for (const xiiOpenDdlReaderElement* pObject = pRoot->GetFirstChild(); pObject != nullptr; pObject = pObject->GetSibling())
  {
    const xiiOpenDdlReaderElement* pGuid        = pObject->FindChildOfType(xiiOpenDdlPrimitiveType::Custom, "id");
    const xiiOpenDdlReaderElement* pType        = pObject->FindChildOfType(xiiOpenDdlPrimitiveType::String, "t");
    const xiiOpenDdlReaderElement* pTypeVersion = pObject->FindChildOfType(xiiOpenDdlPrimitiveType::UInt32, "v");
    const xiiOpenDdlReaderElement* pName        = pObject->FindChildOfType(xiiOpenDdlPrimitiveType::String, "n");
    const xiiOpenDdlReaderElement* pProps       = pObject->FindChildOfType("p");

    if (pGuid == nullptr || pType == nullptr || pProps == nullptr)
    {
      XII_REPORT_FAILURE("Object contains invalid elements");
      continue;
    }

    xiiUuid guid;
    if (xiiOpenDdlUtils::ConvertToUuid(pGuid, guid).Failed())
    {
      XII_REPORT_FAILURE("Object has an invalid guid");
      continue;
    }

    tmp = pType->GetPrimitivesString()[0];

    if (pName)
      tmp2 = pName->GetPrimitivesString()[0];
    else
      tmp2.Clear();

    xiiUInt32 uiTypeVersion = 0;
    if (pTypeVersion)
    {
      uiTypeVersion = pTypeVersion->GetPrimitivesUInt32()[0];
    }

    auto* pNode = pGraph->AddNode(guid, tmp, uiTypeVersion, tmp2);

    for (const xiiOpenDdlReaderElement* pProp = pProps->GetFirstChild(); pProp != nullptr; pProp = pProp->GetSibling())
    {
      if (!pProp->HasName())
        continue;

      if (xiiOpenDdlUtils::ConvertToVariant(pProp, varTmp).Failed())
        continue;

      pNode->AddProperty(pProp->GetName(), varTmp);
    }
  }
}

xiiResult xiiAbstractGraphDdlSerializer::Read(xiiStreamReader& ref_stream, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectGraph* pTypesGraph, bool bApplyPatches)
{
  xiiOpenDdlReader reader;
  if (reader.ParseDocument(ref_stream, 0, xiiLog::GetThreadLocalLogSystem()).Failed())
  {
    xiiLog::Error("Failed to parse DDL graph");
    return XII_FAILURE;
  }

  return Read(reader.GetRootElement(), pGraph, pTypesGraph, bApplyPatches);
}

xiiResult xiiAbstractGraphDdlSerializer::Read(const xiiOpenDdlReaderElement* pRootElement, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectGraph* pTypesGraph /*= nullptr*/, bool bApplyPatches /*= true*/)
{
  const xiiOpenDdlReaderElement* pObjects = pRootElement->FindChildOfType("Objects");
  if (pObjects != nullptr)
  {
    ReadGraph(pGraph, pObjects);
  }
  else
  {
    xiiLog::Error("DDL graph does not contain an 'Objects' root object");
    return XII_FAILURE;
  }

  xiiAbstractObjectGraph* pTempTypesGraph = pTypesGraph;
  if (pTempTypesGraph == nullptr)
  {
    pTempTypesGraph = XII_DEFAULT_NEW(xiiAbstractObjectGraph);
  }
  const xiiOpenDdlReaderElement* pTypes = pRootElement->FindChildOfType("Types");
  if (pTypes != nullptr)
  {
    ReadGraph(pTempTypesGraph, pTypes);
  }

  if (bApplyPatches)
  {
    if (pTempTypesGraph)
      xiiGraphVersioning::GetSingleton()->PatchGraph(pTempTypesGraph);
    xiiGraphVersioning::GetSingleton()->PatchGraph(pGraph, pTempTypesGraph);
  }

  if (pTypesGraph == nullptr)
    XII_DEFAULT_DELETE(pTempTypesGraph);

  return XII_SUCCESS;
}

xiiResult xiiAbstractGraphDdlSerializer::ReadBlocks(xiiStreamReader& stream, xiiHybridArray<xiiSerializedBlock, 3>& blocks)
{
  xiiOpenDdlReader reader;
  if (reader.ParseDocument(stream, 0, xiiLog::GetThreadLocalLogSystem()).Failed())
  {
    xiiLog::Error("Failed to parse DDL graph");
    return XII_FAILURE;
  }

  const xiiOpenDdlReaderElement* pRoot = reader.GetRootElement();
  for (const xiiOpenDdlReaderElement* pChild = pRoot->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
  {
    xiiSerializedBlock* pBlock = GetOrCreateBlock(blocks, pChild->GetCustomType());
    ReadGraph(pBlock->m_Graph.Borrow(), pChild);
  }
  return XII_SUCCESS;
}

#define XII_DOCUMENT_VERSION 2

void xiiAbstractGraphDdlSerializer::WriteDocument(xiiStreamWriter& ref_stream, const xiiAbstractObjectGraph* pHeader, const xiiAbstractObjectGraph* pGraph, const xiiAbstractObjectGraph* pTypes, bool bCompactMode, xiiOpenDdlWriter::TypeStringMode typeMode)
{
  xiiOpenDdlWriter writer;
  writer.SetOutputStream(&ref_stream);
  writer.SetCompactMode(bCompactMode);
  writer.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Exact);
  writer.SetPrimitiveTypeStringMode(typeMode);

  if (typeMode != xiiOpenDdlWriter::TypeStringMode::Compliant)
    writer.SetIndentation(-1);

  xiiStringBuilder sHeaderVersion;
  sHeaderVersion.SetFormat("HeaderV{0}", (xiiInt32)XII_DOCUMENT_VERSION);
  WriteGraph(writer, pHeader, sHeaderVersion);
  WriteGraph(writer, pGraph, "Objects");
  WriteGraph(writer, pTypes, "Types");
}

xiiResult xiiAbstractGraphDdlSerializer::ReadDocument(xiiStreamReader& ref_stream, xiiUniquePtr<xiiAbstractObjectGraph>& ref_pHeader, xiiUniquePtr<xiiAbstractObjectGraph>& ref_pGraph, xiiUniquePtr<xiiAbstractObjectGraph>& ref_pTypes, bool bApplyPatches)
{
  xiiHybridArray<xiiSerializedBlock, 3> blocks;
  if (ReadBlocks(ref_stream, blocks).Failed())
  {
    return XII_FAILURE;
  }

  xiiInt32            iVersion = 2;
  xiiSerializedBlock* pHB      = FindHeaderBlock(blocks, iVersion);
  xiiSerializedBlock* pOB      = FindBlock(blocks, "Objects");
  xiiSerializedBlock* pTB      = FindBlock(blocks, "Types");
  if (!pOB)
  {
    xiiLog::Error("No 'Objects' block in document");
    return XII_FAILURE;
  }
  if (!pTB && !pHB)
  {
    iVersion = 0;
  }
  else if (!pHB)
  {
    iVersion = 1;
  }
  if (iVersion < 2)
  {
    // Move header into its own graph.
    xiiStringBuilder sHeaderVersion;
    sHeaderVersion.SetFormat("HeaderV{0}", iVersion);
    pHB                           = GetOrCreateBlock(blocks, sHeaderVersion);
    xiiAbstractObjectGraph& graph = *pOB->m_Graph.Borrow();
    if (auto* pHeaderNode = graph.GetNodeByName("Header"))
    {
      xiiAbstractObjectGraph& headerGraph = *pHB->m_Graph.Borrow();
      /*auto* pNewHeaderNode =*/headerGraph.CopyNodeIntoGraph(pHeaderNode);
      // pNewHeaderNode->AddProperty("DocVersion", iVersion);
      graph.RemoveNode(pHeaderNode->GetGuid());
    }
  }

  if (bApplyPatches && pTB)
  {
    xiiGraphVersioning::GetSingleton()->PatchGraph(pTB->m_Graph.Borrow());
    xiiGraphVersioning::GetSingleton()->PatchGraph(pHB->m_Graph.Borrow(), pTB->m_Graph.Borrow());
    xiiGraphVersioning::GetSingleton()->PatchGraph(pOB->m_Graph.Borrow(), pTB->m_Graph.Borrow());
  }

  ref_pHeader = std::move(pHB->m_Graph);
  ref_pGraph  = std::move(pOB->m_Graph);
  if (pTB)
  {
    ref_pTypes = std::move(pTB->m_Graph);
  }

  return XII_SUCCESS;
}

// This is a handcrafted DDL reader that ignores everything that is not an 'AssetInfo' object
// The purpose is to speed up reading asset information by skipping everything else
//
// Version 0 and 1:
// The reader 'knows' the file format details and uses them.
// Top-level (ie. depth 0) there is an "Objects" object -> we need to enter that
// Inside that (depth 1) there is the "AssetInfo" object -> need to enter that as well
// All objects inside that must be stored
// Once the AssetInfo object is left everything else can be skipped
//
// Version 2:
// The very first top level object is "Header" and only that is read and parsing is stopped afterwards.
class HeaderReader : public xiiOpenDdlReader
{
public:
  HeaderReader() = default;

  bool     m_bHasHeader = false;
  xiiInt32 m_iDepth     = 0;

  virtual void OnBeginObject(xiiStringView sType, xiiStringView sName, bool bGlobalName) override
  {
    //////////////////////////////////////////////////////////////////////////
    // New document format has header block.
    if (m_iDepth == 0 && sType.StartsWith("HeaderV"))
    {
      m_bHasHeader = true;
    }
    if (m_bHasHeader)
    {
      ++m_iDepth;
      xiiOpenDdlReader::OnBeginObject(sType, sName, bGlobalName);
      return;
    }

    //////////////////////////////////////////////////////////////////////////
    // Old header is stored in the object block.
    // not yet entered the "Objects" group
    if (m_iDepth == 0 && sType == "Objects")
    {
      ++m_iDepth;

      xiiOpenDdlReader::OnBeginObject(sType, sName, bGlobalName);
      return;
    }

    // not yet entered the "AssetInfo" group, but inside "Objects"
    if (m_iDepth == 1 && sType == "AssetInfo")
    {
      ++m_iDepth;

      xiiOpenDdlReader::OnBeginObject(sType, sName, bGlobalName);
      return;
    }

    // inside "AssetInfo"
    if (m_iDepth > 1)
    {
      ++m_iDepth;
      xiiOpenDdlReader::OnBeginObject(sType, sName, bGlobalName);
      return;
    }

    // ignore everything else
    SkipRestOfObject();
  }

  virtual void OnEndObject() override
  {
    --m_iDepth;
    if (m_bHasHeader)
    {
      if (m_iDepth == 0)
      {
        m_iDepth = -1;
        StopParsing();
      }
    }
    else
    {
      if (m_iDepth <= 1)
      {
        // we were inside "AssetInfo" or "Objects" and returned from it, so now skip the rest
        m_iDepth = -1;
        StopParsing();
      }
    }
    xiiOpenDdlReader::OnEndObject();
  }
};

xiiResult xiiAbstractGraphDdlSerializer::ReadHeader(xiiStreamReader& ref_stream, xiiAbstractObjectGraph* pGraph)
{
  HeaderReader reader;
  if (reader.ParseDocument(ref_stream, 0, xiiLog::GetThreadLocalLogSystem()).Failed())
  {
    XII_REPORT_FAILURE("Failed to parse DDL graph");
    return XII_FAILURE;
  }

  const xiiOpenDdlReaderElement* pObjects = nullptr;
  if (reader.m_bHasHeader)
  {
    pObjects = reader.GetRootElement()->GetFirstChild();
  }
  else
  {
    pObjects = reader.GetRootElement()->FindChildOfType("Objects");
  }

  if (pObjects != nullptr)
  {
    ReadGraph(pGraph, pObjects);
  }
  else
  {
    XII_REPORT_FAILURE("DDL graph does not contain an 'Objects' root object");
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_DdlSerializer);
