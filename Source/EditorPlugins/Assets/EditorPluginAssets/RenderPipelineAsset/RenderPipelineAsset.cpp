/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/BuiltinRenderPipelinePass.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineAssetDocument, 5, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  class xiiPipelineEditorPin : public xiiPin
  {
  public:
    xiiPipelineEditorPin(const xiiRenderPipelinePin& pin, const xiiDocumentObject* object) :
      xiiPin(pin.m_bInput ? Type::Input : Type::Output, pin.m_sName, xiiColorScheme::DarkUI(pin.m_Type == xiiRenderPipelinePin::Texture ? xiiColorScheme::Blue : pin.m_Type == xiiRenderPipelinePin::Buffer ? xiiColorScheme::Green :
                                                                                                                                                                                                              xiiColorScheme::Gray),
             object),
      m_Schema(pin)
    {
    }

    xiiRenderPipelinePin m_Schema;
  };
} // namespace

void xiiRenderPipelineNodeManager::GetNodeCreationTemplates(xiiDynamicArray<xiiNodeCreationTemplate>& out_templates) const
{
  out_templates.Clear();
  xiiHybridArray<const xiiRTTI*, 32> types;
  GetCreateableTypes(types);
  for (const auto* type : types)
  {
    if (type == xiiGetStaticRTTI<xiiBuiltinRenderPipelinePass>())
      continue;
    auto& entry       = out_templates.ExpandAndGetRef();
    entry.m_pType     = type;
    entry.m_sTypeName = type->GetTypeName();
  }
  const auto stages = xiiBuiltinRenderPipelinePass::GetStages();
  m_BuiltinProperties.Clear();
  m_BuiltinProperties.Reserve(stages.GetCount());
  for (const auto stage : stages)
  {
    auto& setting = m_BuiltinProperties.ExpandAndGetRef();
    setting.m_sPropertyName.Assign("Stage");
    setting.m_Value   = stage;
    auto& entry       = out_templates.ExpandAndGetRef();
    entry.m_pType     = xiiGetStaticRTTI<xiiBuiltinRenderPipelinePass>();
    entry.m_sTypeName = stage;
    entry.m_sCategory.Assign("Built-in Rendering");
    entry.m_PropertyValues = xiiArrayPtr<const xiiNodePropertyValue>(&setting, 1);
  }
}

bool xiiRenderPipelineNodeManager::InternalIsNode(const xiiDocumentObject* pObject) const
{
  return pObject->GetTypeAccessor().GetType()->IsDerivedFrom<xiiRenderPipelinePass>();
}

bool xiiRenderPipelineNodeManager::InternalIsDynamicPinProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp) const
{
  // A reflected setting may change the native pass schema. The node manager handles pin recreation.
  return InternalIsNode(pObject) && pProp->GetCategory() == xiiPropertyCategory::Member;
}

void xiiRenderPipelineNodeManager::InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& ref_node)
{
  const auto* type = xiiRTTI::FindTypeByName(pObject->GetTypeAccessor().GetType()->GetTypeName());
  if (!type || !type->IsDerivedFrom<xiiRenderPipelinePass>() || !type->GetAllocator()->CanAllocate())
    return;
  xiiRenderPipelinePass* pass = type->GetAllocator()->Allocate<xiiRenderPipelinePass>();
  XII_SCOPE_EXIT(type->GetAllocator()->Deallocate(pass));
  xiiVariantDictionary                           settings;
  xiiHybridArray<const xiiAbstractProperty*, 32> properties;
  type->GetAllProperties(properties);
  for (const auto* property : properties)
    if (property->GetCategory() == xiiPropertyCategory::Member && !property->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
      settings.Insert(property->GetPropertyName(), pObject->GetTypeAccessor().GetValue(property->GetPropertyName()));
  if (pass->Configure(settings).Failed())
    return;
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  pass->GetPins(pins);
  for (const auto& pin : pins)
  {
    auto editorPin = XII_DEFAULT_NEW(xiiPipelineEditorPin, pin, pObject);
    if (pin.m_bInput)
      ref_node.m_Inputs.PushBack(std::move(editorPin));
    else
      ref_node.m_Outputs.PushBack(std::move(editorPin));
  }
}

void xiiRenderPipelineNodeManager::GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& ref_types) const
{
  xiiSet<const xiiRTTI*> types;
  xiiReflectionUtils::GatherTypesDerivedFromClass(xiiGetStaticRTTI<xiiRenderPipelinePass>(), types);
  ref_types.Clear();
  for (const auto* type : types)
    if (!type->GetTypeFlags().IsAnySet(xiiTypeFlags::Abstract) && type->GetAllocator()->CanAllocate())
      ref_types.PushBack(type);
}

xiiStatus xiiRenderPipelineNodeManager::InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_result) const
{
  out_result         = CanConnectResult::ConnectNever;
  const auto& output = static_cast<const xiiPipelineEditorPin&>(source).m_Schema;
  const auto& input  = static_cast<const xiiPipelineEditorPin&>(target).m_Schema;
  if (output.m_Type != input.m_Type || (output.m_Type == xiiRenderPipelinePin::Texture && output.m_Format != xiiGALResourceFormat::Unknown && input.m_Format != xiiGALResourceFormat::Unknown && output.m_Format != input.m_Format))
    return xiiStatus("Render pipeline pin resource types or formats do not match.");
  if (WouldConnectionCreateCircle(source, target))
    return xiiStatus("Render pipeline connections must form a directed acyclic graph.");
  out_result = input.m_Type == xiiRenderPipelinePin::Order ? CanConnectResult::ConnectNtoN : CanConnectResult::ConnectNto1;
  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

xiiRenderPipelineAssetDocument::xiiRenderPipelineAssetDocument(xiiStringView sDocumentPath) :
  xiiAssetDocument(sDocumentPath, XII_DEFAULT_NEW(xiiRenderPipelineNodeManager), xiiAssetDocEngineConnection::FullObjectMirroring)
{
}

xiiRenderPipelineAssetDocument::~xiiRenderPipelineAssetDocument() = default;

xiiTransformStatus xiiRenderPipelineAssetDocument::InternalTransformAsset(xiiStringView sTargetFile, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  return xiiAssetDocument::RemoteExport(AssetHeader, sTargetFile);
}

xiiTransformStatus xiiRenderPipelineAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  XII_REPORT_FAILURE("Should not be called");
  return xiiTransformStatus();
}

void xiiRenderPipelineAssetDocument::InternalGetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const
{
  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());

  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void xiiRenderPipelineAssetDocument::AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);

  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());

  pManager->AttachMetaDataBeforeSaving(graph);
}

void xiiRenderPipelineAssetDocument::RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);

  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());

  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}

void xiiRenderPipelineAssetDocument::GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/xiiEditor.RenderPipelineGraph");
}

bool xiiRenderPipelineAssetDocument::CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_MimeType) const
{
  out_MimeType = "application/xiiEditor.RenderPipelineGraph";

  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());

  return pManager->CopySelectedObjects(out_objectGraph);
}

bool xiiRenderPipelineAssetDocument::Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, xiiStringView sMimeType)
{
  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());

  return pManager->PasteObjects(info, objectGraph, xiiQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}
