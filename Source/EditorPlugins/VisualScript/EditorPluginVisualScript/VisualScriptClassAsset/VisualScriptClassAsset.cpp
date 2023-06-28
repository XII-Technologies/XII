#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

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
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptClassAssetDocument, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptClassAssetDocument::xiiVisualScriptClassAssetDocument(const char* szDocumentPath) :
  xiiSimpleAssetDocument<xiiVisualScriptClassAssetProperties>(XII_DEFAULT_NEW(xiiVisualScriptNodeManager), szDocumentPath, xiiAssetDocEngineConnection::None)
{
  m_pObjectAccessor = XII_DEFAULT_NEW(xiiNodeCommandAccessor, GetCommandHistory());
}

xiiTransformStatus xiiVisualScriptClassAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
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

  xiiHybridArray<const xiiVisualScriptPin*, 16> pins;
  xiiVisualScriptCompiler                       compiler;

  for (const xiiDocumentObject* pObject : children)
  {
    if (pManager->IsNode(pObject) == false)
      continue;

    auto pNodeDesc = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    if (pNodeDesc == nullptr)
      return xiiStatus(XII_FAILURE);

    if (pManager->IsFilteredByBaseClass(pObject->GetType(), *pNodeDesc, sBaseClass, true))
      continue;

    const char* szFunctionName = xiiStringUtils::FindLastSubString(pObject->GetType()->GetTypeName(), "::");
    if (szFunctionName != nullptr)
    {
      szFunctionName += 2;
    }

    for (auto& eventHandler : compiler.GetCompiledModule().m_Functions)
    {
      if (eventHandler.m_sName == szFunctionName)
      {
        return xiiStatus(xiiFmt("A event handler for '{}' already exists. Can't have multiple event handler for the same event.", szFunctionName));
      }
    }

    if (pNodeDesc->m_Type == xiiVisualScriptNodeDescription::Type::EntryCall ||
        pNodeDesc->m_Type == xiiVisualScriptNodeDescription::Type::MessageHandler)
    {
      pManager->GetOutputExecutionPins(pObject, pins);
      if (pins.IsEmpty())
        continue;

      if (pManager->GetConnections(*pins[0]).IsEmpty())
        continue;

      XII_SUCCEED_OR_RETURN(compiler.AddFunction(szFunctionName, pNodeDesc->m_Type, pObject));
    }
  }

  xiiStringBuilder sDumpPath;
  if (false)
  {
    sDumpPath.Format(":appdata/{}_AST.dgml", sScriptClassName);
  }
  XII_SUCCEED_OR_RETURN(compiler.Compile(sDumpPath));

  auto& compiledModule = compiler.GetCompiledModule();
  XII_SUCCEED_OR_RETURN(compiledModule.Serialize(stream, sBaseClassName, sScriptClassName));

  return xiiStatus(XII_SUCCESS);
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

bool xiiVisualScriptClassAssetDocument::Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType)
{
  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, xiiQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}
