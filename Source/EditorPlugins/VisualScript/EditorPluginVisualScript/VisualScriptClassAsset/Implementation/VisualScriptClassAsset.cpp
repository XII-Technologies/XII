#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginVisualScript/VisualScriptClassAsset/VisualScriptClassAsset.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptCompiler.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <ToolsFoundation/NodeObject/NodeCommandAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptClassAssetProperties, 1, xiiRTTIDefaultAllocator<xiiVisualScriptClassAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("BaseClass", m_sBaseClass)->AddAttributes(new xiiDefaultValueAttribute(xiiStringView("Component")), new xiiDynamicStringEnumAttribute("ScriptBaseClasses")),
    XII_ARRAY_MEMBER_PROPERTY("Variables", m_Variables),
    XII_MEMBER_PROPERTY("DumpAST", m_bDumpAST),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptClassAssetDocument, 7, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptClassAssetDocument::xiiVisualScriptClassAssetDocument(xiiStringView sDocumentPath) :
  xiiSimpleAssetDocument<xiiVisualScriptClassAssetProperties>(XII_DEFAULT_NEW(xiiVisualScriptNodeManager), sDocumentPath, xiiAssetDocEngineConnection::None)
{
  m_pObjectAccessor = XII_DEFAULT_NEW(xiiNodeCommandAccessor, GetCommandHistory());
}

xiiTransformStatus xiiVisualScriptClassAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  auto pManager = static_cast<xiiVisualScriptNodeManager*>(GetObjectManager());

  const auto&     children   = pManager->GetRootObject()->GetChildren();
  xiiHashedString sBaseClass = pManager->GetScriptBaseClass();

  xiiStringBuilder sBaseClassName = sBaseClass.GetView();
  if (xiiRTTI::FindTypeByName(sBaseClassName) == nullptr)
  {
    sBaseClassName.Prepend("xii");
    if (xiiRTTI::FindTypeByName(sBaseClassName) == nullptr)
    {
      return xiiStatus(xiiFmt("Invalid base class '{}'", sBaseClassName));
    }
  }

  xiiStringView sScriptClassName = xiiPathUtils::GetFileName(GetDocumentPath());

  xiiVisualScriptCompiler compiler;
  compiler.InitModule(sBaseClassName, sScriptClassName);

  xiiHybridArray<const xiiVisualScriptPin*, 16> pins;
  for (const xiiDocumentObject* pObject : children)
  {
    if (pManager->IsNode(pObject) == false)
      continue;

    auto pNodeDesc = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    if (pNodeDesc == nullptr)
      return xiiStatus(XII_FAILURE);

    if (pManager->IsFilteredByBaseClass(pObject->GetType(), *pNodeDesc, sBaseClass, true))
      continue;

    if (xiiVisualScriptNodeDescription::Type::IsEntry(pNodeDesc->m_Type))
    {
      pManager->GetOutputExecutionPins(pObject, pins);
      if (pins.IsEmpty())
        continue;

      if (pManager->GetConnections(*pins[0]).IsEmpty())
        continue;

      xiiStringView sFunctionName = xiiVisualScriptNodeManager::GetNiceFunctionName(pObject);
      XII_SUCCEED_OR_RETURN(compiler.AddFunction(sFunctionName, pObject));
    }
  }

  xiiStringBuilder sDumpPath;
  if (GetProperties()->m_bDumpAST)
  {
    sDumpPath.SetFormat(":appdata/{}_AST.dgml", sScriptClassName);
  }
  XII_SUCCEED_OR_RETURN(compiler.Compile(sDumpPath));

  auto& compiledModule = compiler.GetCompiledModule();
  XII_SUCCEED_OR_RETURN(compiledModule.Serialize(stream));

  return xiiStatus(XII_SUCCESS);
}

void xiiVisualScriptClassAssetDocument::UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  xiiExposedParameters* pExposedParams = XII_DEFAULT_NEW(xiiExposedParameters);

  for (const auto& v : GetProperties()->m_Variables)
  {
    if (v.m_bExpose == false)
      continue;

    xiiExposedParameter* param = XII_DEFAULT_NEW(xiiExposedParameter);
    param->m_sName             = v.m_sName.GetString();
    param->m_DefaultValue      = v.m_DefaultValue;

    pExposedParams->m_Parameters.PushBack(param);
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

void xiiVisualScriptClassAssetDocument::InternalGetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const
{
  auto pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void xiiVisualScriptClassAssetDocument::AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const auto pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void xiiVisualScriptClassAssetDocument::RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  auto pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}

void xiiVisualScriptClassAssetDocument::GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/xiiEditor.VisualScriptClassGraph");
}

bool xiiVisualScriptClassAssetDocument::CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_MimeType) const
{
  out_MimeType = "application/xiiEditor.VisualScriptClassGraph";

  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool xiiVisualScriptClassAssetDocument::Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, xiiStringView sMimeType)
{
  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, xiiQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}
