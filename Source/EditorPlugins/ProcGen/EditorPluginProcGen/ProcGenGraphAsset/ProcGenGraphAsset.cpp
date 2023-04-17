#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodeManager.h>
#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

namespace
{
  void DumpAST(const xiiExpressionAST& ast, xiiStringView sAssetName, xiiStringView sOutputName)
  {
    xiiDGMLGraph dgmlGraph;
    ast.PrintGraph(dgmlGraph);

    xiiStringBuilder sFileName;
    sFileName.Format(":appdata/{0}_{1}_AST.dgml", sAssetName, sOutputName);

    xiiDGMLGraphWriter dgmlGraphWriter;
    XII_IGNORE_UNUSED(dgmlGraphWriter);
    if (dgmlGraphWriter.WriteGraphToFile(sFileName, dgmlGraph).Succeeded())
    {
      xiiLog::Info("AST was dumped to: {0}", sFileName);
    }
    else
    {
      xiiLog::Error("Failed to dump AST to: {0}", sFileName);
    }
  }

  static const char* s_szSphereAssetId     = "{ a3ce5d3d-be5e-4bda-8820-b1ce3b3d33fd }"; // Base/Prefabs/Sphere.xiiPrefab
  static const char* s_szBWGradientAssetId = "{ 3834b7d0-5a3f-140d-31d8-3a2bf48b09bd }"; // Base/Textures/BlackWhiteGradient.xiiColorGradientAsset

} // namespace

////////////////////////////////////////////////////////////////

struct DocObjAndOutput
{
  XII_DECLARE_POD_TYPE();

  const xiiDocumentObject* m_pObject;
  const char*              m_szOutputName;
};

template <>
struct xiiHashHelper<DocObjAndOutput>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(const DocObjAndOutput& value)
  {
    const xiiUInt32 hashA = xiiHashHelper<const void*>::Hash(value.m_pObject);
    const xiiUInt32 hashB = xiiHashHelper<const void*>::Hash(value.m_szOutputName);
    return xiiHashingUtils::CombineHashValues32(hashA, hashB);
  }

  XII_ALWAYS_INLINE static bool Equal(const DocObjAndOutput& a, const DocObjAndOutput& b)
  {
    return a.m_pObject == b.m_pObject && a.m_szOutputName == b.m_szOutputName;
  }
};

struct xiiProcGenGraphAssetDocument::GenerateContext
{
  GenerateContext(const xiiDocumentObjectManager* pManager) :
    m_ObjectWriter(&m_AbstractObjectGraph, pManager), m_RttiConverter(&m_AbstractObjectGraph, &m_RttiConverterContext)
  {
  }

  xiiAbstractObjectGraph                                                   m_AbstractObjectGraph;
  xiiDocumentObjectConverterWriter                                         m_ObjectWriter;
  xiiRttiConverterContext                                                  m_RttiConverterContext;
  xiiRttiConverterReader                                                   m_RttiConverter;
  xiiHashTable<const xiiDocumentObject*, xiiUniquePtr<xiiProcGenNodeBase>> m_DocObjToProcGenNodeTable;
  xiiHashTable<DocObjAndOutput, xiiExpressionAST::Node*>                   m_DocObjAndOutputToASTNodeTable;
  xiiProcGenNodeBase::GraphContext                                         m_GraphContext;
};

////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcGenGraphAssetDocument, 6, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiProcGenGraphAssetDocument::xiiProcGenGraphAssetDocument(const char* szDocumentPath) :
  xiiAssetDocument(szDocumentPath, XII_DEFAULT_NEW(xiiProcGenNodeManager), xiiAssetDocEngineConnection::None)
{
}

void xiiProcGenGraphAssetDocument::SetDebugPin(const xiiPin* pDebugPin)
{
  m_pDebugPin = pDebugPin;

  if (m_pDebugPin != nullptr)
  {
    CreateDebugNode();
  }

  xiiDocumentObjectPropertyEvent e;
  e.m_EventType = xiiDocumentObjectPropertyEvent::Type::PropertySet;
  e.m_sProperty = "DebugPin";

  GetObjectManager()->m_PropertyEvents.Broadcast(e);
}

xiiStatus xiiProcGenGraphAssetDocument::WriteAsset(xiiStreamWriter& stream, const xiiPlatformProfile* pAssetProfile, bool bAllowDebug) const
{
  GenerateContext context(GetObjectManager());

  xiiDynamicArray<const xiiDocumentObject*> placementNodes;
  xiiDynamicArray<const xiiDocumentObject*> vertexColorNodes;
  GetAllOutputNodes(placementNodes, vertexColorNodes);

  const bool bDebug = bAllowDebug && (m_pDebugPin != nullptr);

  xiiStringDeduplicationWriteContext stringDedupContext(stream);

  xiiChunkStreamWriter chunk(stringDedupContext.Begin());
  chunk.BeginStream(1);

  xiiExpressionCompiler compiler;

  auto WriteByteCode = [&](const xiiDocumentObject* pOutputNode) {
    context.m_GraphContext.m_VolumeTagSetIndices.Clear();

    if (pOutputNode->GetType()->IsDerivedFrom<xiiProcGen_PlacementOutput>())
    {
      context.m_GraphContext.m_OutputType = xiiProcGenNodeBase::GraphContext::Placement;
    }
    else if (pOutputNode->GetType()->IsDerivedFrom<xiiProcGen_VertexColorOutput>())
    {
      context.m_GraphContext.m_OutputType = xiiProcGenNodeBase::GraphContext::Color;
    }
    else
    {
      XII_ASSERT_NOT_IMPLEMENTED;
      return xiiStatus("Unknown output type");
    }

    xiiExpressionAST ast;
    GenerateExpressionAST(pOutputNode, "", context, ast);
    context.m_DocObjAndOutputToASTNodeTable.Clear();

    if (false)
    {
      xiiStringBuilder sDocumentPath = GetDocumentPath();
      xiiStringView    sAssetName    = sDocumentPath.GetFileNameAndExtension();
      xiiStringView    sOutputName   = pOutputNode->GetTypeAccessor().GetValue("Name").ConvertTo<xiiString>();

      DumpAST(ast, sAssetName, sOutputName);
    }

    xiiExpressionByteCode byteCode;
    if (compiler.Compile(ast, byteCode).Failed())
    {
      return xiiStatus("Compilation failed");
    }

    byteCode.Save(chunk);

    return xiiStatus(XII_SUCCESS);
  };

  {
    chunk.BeginChunk("PlacementOutputs", 6);

    if (!bDebug)
    {
      chunk << placementNodes.GetCount();

      for (auto pPlacementNode : placementNodes)
      {
        XII_SUCCEED_OR_RETURN(WriteByteCode(pPlacementNode));

        auto pPGNode          = context.m_DocObjToProcGenNodeTable.GetValue(pPlacementNode);
        auto pPlacementOutput = xiiStaticCast<xiiProcGen_PlacementOutput*>(pPGNode->Borrow());

        pPlacementOutput->m_VolumeTagSetIndices = context.m_GraphContext.m_VolumeTagSetIndices;
        pPlacementOutput->Save(chunk);
      }
    }
    else
    {
      xiiUInt32 uiNumNodes = 1;
      chunk << uiNumNodes;

      context.m_GraphContext.m_VolumeTagSetIndices.Clear();
      context.m_GraphContext.m_OutputType = xiiProcGenNodeBase::GraphContext::Placement;

      xiiExpressionAST ast;
      GenerateDebugExpressionAST(context, ast);
      context.m_DocObjAndOutputToASTNodeTable.Clear();

      xiiExpressionByteCode byteCode;
      if (compiler.Compile(ast, byteCode).Failed())
      {
        return xiiStatus("Debug Compilation failed");
      }

      byteCode.Save(chunk);

      m_pDebugNode->m_VolumeTagSetIndices = context.m_GraphContext.m_VolumeTagSetIndices;
      m_pDebugNode->Save(chunk);
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("VertexColorOutputs", 2);

    chunk << vertexColorNodes.GetCount();

    for (auto pVertexColorNode : vertexColorNodes)
    {
      XII_SUCCEED_OR_RETURN(WriteByteCode(pVertexColorNode));

      auto pPGNode            = context.m_DocObjToProcGenNodeTable.GetValue(pVertexColorNode);
      auto pVertexColorOutput = xiiStaticCast<xiiProcGen_VertexColorOutput*>(pPGNode->Borrow());

      pVertexColorOutput->m_VolumeTagSetIndices = context.m_GraphContext.m_VolumeTagSetIndices;
      pVertexColorOutput->Save(chunk);
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("SharedData", 1);

    context.m_GraphContext.m_SharedData.Save(chunk);

    chunk.EndChunk();
  }

  chunk.EndStream();
  XII_SUCCEED_OR_RETURN(stringDedupContext.End());

  return xiiStatus(XII_SUCCESS);
}

void xiiProcGenGraphAssetDocument::UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  if (m_pDebugPin == nullptr)
  {
    xiiDynamicArray<const xiiDocumentObject*> placementNodes;
    xiiDynamicArray<const xiiDocumentObject*> vertexColorNodes;
    GetAllOutputNodes(placementNodes, vertexColorNodes);

    for (auto pPlacementNode : placementNodes)
    {
      auto& typeAccessor = pPlacementNode->GetTypeAccessor();

      xiiUInt32 uiNumObjects = typeAccessor.GetCount("Objects");
      for (xiiUInt32 i = 0; i < uiNumObjects; ++i)
      {
        xiiVariant prefab = typeAccessor.GetValue("Objects", i);
        if (prefab.IsA<xiiString>())
        {
          pInfo->m_PackageDependencies.Insert(prefab.Get<xiiString>());
          pInfo->m_ThumbnailDependencies.Insert(prefab.Get<xiiString>());
        }
      }

      xiiVariant colorGradient = typeAccessor.GetValue("ColorGradient");
      if (colorGradient.IsA<xiiString>())
      {
        pInfo->m_PackageDependencies.Insert(colorGradient.Get<xiiString>());
        pInfo->m_ThumbnailDependencies.Insert(colorGradient.Get<xiiString>());
      }
    }
  }
  else
  {
    pInfo->m_PackageDependencies.Insert(s_szSphereAssetId);
    pInfo->m_PackageDependencies.Insert(s_szBWGradientAssetId);

    pInfo->m_ThumbnailDependencies.Insert(s_szSphereAssetId);
    pInfo->m_ThumbnailDependencies.Insert(s_szBWGradientAssetId);
  }
}

xiiTransformStatus xiiProcGenGraphAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  XII_ASSERT_DEV(xiiStringUtils::IsNullOrEmpty(szOutputTag), "Additional output '{0}' not implemented!", szOutputTag);

  return WriteAsset(stream, pAssetProfile, false);
}

void xiiProcGenGraphAssetDocument::GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/xiiEditor.ProcGenGraph");
}

bool xiiProcGenGraphAssetDocument::CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_MimeType) const
{
  out_MimeType = "application/xiiEditor.ProcGenGraph";

  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool xiiProcGenGraphAssetDocument::Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType)
{
  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, xiiQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}

void xiiProcGenGraphAssetDocument::AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void xiiProcGenGraphAssetDocument::RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}

void xiiProcGenGraphAssetDocument::GetAllOutputNodes(xiiDynamicArray<const xiiDocumentObject*>& placementNodes, xiiDynamicArray<const xiiDocumentObject*>& vertexColorNodes) const
{
  const xiiRTTI* pPlacementOutputRtti   = xiiGetStaticRTTI<xiiProcGen_PlacementOutput>();
  const xiiRTTI* pVertexColorOutputRtti = xiiGetStaticRTTI<xiiProcGen_VertexColorOutput>();

  placementNodes.Clear();
  vertexColorNodes.Clear();

  const auto& children = GetObjectManager()->GetRootObject()->GetChildren();
  for (const xiiDocumentObject* pObject : children)
  {
    if (pObject->GetTypeAccessor().GetValue("Active").ConvertTo<bool>())
    {
      const xiiRTTI* pRtti = pObject->GetTypeAccessor().GetType();
      if (pRtti->IsDerivedFrom(pPlacementOutputRtti))
      {
        placementNodes.PushBack(pObject);
      }
      else if (pRtti->IsDerivedFrom(pVertexColorOutputRtti))
      {
        vertexColorNodes.PushBack(pObject);
      }
    }
  }
}

void xiiProcGenGraphAssetDocument::InternalGetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const
{
  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

xiiExpressionAST::Node* xiiProcGenGraphAssetDocument::GenerateExpressionAST(const xiiDocumentObject* outputNode, const char* szOutputName, GenerateContext& context, xiiExpressionAST& out_Ast) const
{
  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());

  auto inputPins = pManager->GetInputPins(outputNode);

  xiiHybridArray<xiiExpressionAST::Node*, 8> inputAstNodes;
  inputAstNodes.SetCount(inputPins.GetCount());

  for (xiiUInt32 i = 0; i < inputPins.GetCount(); ++i)
  {
    auto connections = pManager->GetConnections(*inputPins[i]);
    XII_ASSERT_DEBUG(connections.GetCount() <= 1, "Input pin has {0} connections", connections.GetCount());

    if (connections.IsEmpty())
      continue;

    const xiiPin& pinSource = connections[0]->GetSourcePin();

    DocObjAndOutput         key = {pinSource.GetParent(), pinSource.GetName()};
    xiiExpressionAST::Node* astNode;
    if (!context.m_DocObjAndOutputToASTNodeTable.TryGetValue(key, astNode))
    {
      // recursively generate all dependent code
      astNode = GenerateExpressionAST(pinSource.GetParent(), pinSource.GetName(), context, out_Ast);

      context.m_DocObjAndOutputToASTNodeTable.Insert(key, astNode);
    }

    inputAstNodes[i] = astNode;
  }

  xiiProcGenNodeBase* cachedPGNode = nullptr;
  if (auto pCachedPGNode = context.m_DocObjToProcGenNodeTable.GetValue(outputNode))
  {
    cachedPGNode = pCachedPGNode->Borrow();
  }
  else
  {
    xiiAbstractObjectNode* pAbstractNode = context.m_ObjectWriter.AddObjectToGraph(outputNode);
    auto                   newPGNode     = context.m_RttiConverter.CreateObjectFromNode(pAbstractNode).Cast<xiiProcGenNodeBase>();
    cachedPGNode                         = newPGNode;

    context.m_DocObjToProcGenNodeTable.Insert(outputNode, newPGNode);
  }

  return cachedPGNode->GenerateExpressionASTNode(xiiTempHashedString(szOutputName), inputAstNodes, out_Ast, context.m_GraphContext);
}

xiiExpressionAST::Node* xiiProcGenGraphAssetDocument::GenerateDebugExpressionAST(GenerateContext& context, xiiExpressionAST& out_Ast) const
{
  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  XII_ASSERT_DEV(m_pDebugPin != nullptr, "");

  const xiiPin* pPinSource = m_pDebugPin;
  if (pPinSource->GetType() == xiiPin::Type::Input)
  {
    auto connections = pManager->GetConnections(*pPinSource);
    XII_ASSERT_DEBUG(connections.GetCount() <= 1, "Input pin has {0} connections", connections.GetCount());

    if (connections.IsEmpty())
      return nullptr;

    pPinSource = &connections[0]->GetSourcePin();
    XII_ASSERT_DEBUG(pPinSource != nullptr, "Invalid connection");
  }

  xiiHybridArray<xiiExpressionAST::Node*, 8> inputAstNodes;
  inputAstNodes.SetCount(4); // placement output node has 4 inputs

  // Recursively generate all dependent code and pretend it is connected to the color index input of the debug placement output node.
  inputAstNodes[2] = GenerateExpressionAST(pPinSource->GetParent(), pPinSource->GetName(), context, out_Ast);

  return m_pDebugNode->GenerateExpressionASTNode("", inputAstNodes, out_Ast, context.m_GraphContext);
}

void xiiProcGenGraphAssetDocument::DumpSelectedOutput(bool bAst, bool bDisassembly) const
{
  const xiiDocumentObject* pSelectedNode = nullptr;

  auto selection = GetSelectionManager()->GetSelection();
  if (!selection.IsEmpty())
  {
    pSelectedNode = selection[0];
    if (!pSelectedNode->GetType()->IsDerivedFrom<xiiProcGenOutput>())
    {
      pSelectedNode = nullptr;
    }
  }

  if (pSelectedNode == nullptr)
  {
    xiiLog::Error("No valid output node selected.");
    return;
  }

  GenerateContext context(GetObjectManager());
  if (pSelectedNode->GetType()->IsDerivedFrom<xiiProcGen_PlacementOutput>())
  {
    context.m_GraphContext.m_OutputType = xiiProcGenNodeBase::GraphContext::Placement;
  }
  else if (pSelectedNode->GetType()->IsDerivedFrom<xiiProcGen_VertexColorOutput>())
  {
    context.m_GraphContext.m_OutputType = xiiProcGenNodeBase::GraphContext::Color;
  }
  else
  {
    XII_ASSERT_NOT_IMPLEMENTED;
    return;
  }

  xiiExpressionAST ast;
  GenerateExpressionAST(pSelectedNode, "", context, ast);

  xiiStringBuilder sDocumentPath = GetDocumentPath();
  xiiStringView    sAssetName    = sDocumentPath.GetFileNameAndExtension();
  xiiStringView    sOutputName   = pSelectedNode->GetTypeAccessor().GetValue("Name").ConvertTo<xiiString>();

  if (bAst)
  {
    DumpAST(ast, sAssetName, sOutputName);
  }

  xiiExpressionByteCode byteCode;
  xiiExpressionCompiler compiler;
  if (compiler.Compile(ast, byteCode).Failed())
  {
    xiiLog::Error("Compiling expression failed");
    return;
  }

  if (bAst)
  {
    xiiStringBuilder sOutputName2 = sOutputName;
    sOutputName2.Append("_Opt");

    DumpAST(ast, sAssetName, sOutputName2);
  }

  if (bDisassembly)
  {
    xiiStringBuilder sDisassembly;
    byteCode.Disassemble(sDisassembly);

    xiiStringBuilder sFileName;
    sFileName.Format(":appdata/{0}_{1}_ByteCode.txt", sAssetName, sOutputName);

    xiiFileWriter fileWriter;
    if (fileWriter.Open(sFileName).Succeeded())
    {
      fileWriter.WriteBytes(sDisassembly.GetData(), sDisassembly.GetElementCount()).IgnoreResult();

      xiiLog::Info("Disassembly was dumped to: {0}", sFileName);
    }
    else
    {
      xiiLog::Error("Failed to dump Disassembly to: {0}", sFileName);
    }
  }
}

void xiiProcGenGraphAssetDocument::CreateDebugNode()
{
  if (m_pDebugNode != nullptr)
    return;

  m_pDebugNode          = XII_DEFAULT_NEW(xiiProcGen_PlacementOutput);
  m_pDebugNode->m_sName = "Debug";
  m_pDebugNode->m_ObjectsToPlace.PushBack(s_szSphereAssetId);
  m_pDebugNode->m_sColorGradient = s_szBWGradientAssetId;
}
