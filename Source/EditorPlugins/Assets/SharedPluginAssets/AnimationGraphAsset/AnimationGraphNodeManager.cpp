/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <Foundation/Math/ColorScheme.h>
#include <SharedPluginAssets/AnimationGraphAsset/AnimationGraphNode.h>
#include <SharedPluginAssets/AnimationGraphAsset/AnimationGraphNodeManager.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

void xiiAnimationGraphNodeManager::GetNodeCreationTemplates(xiiDynamicArray<xiiNodeCreationTemplate>& out_templates) const
{
  static constexpr const char* names[] = {"Output", "Clip", "Blend1D", "Additive", "LayeredBlend", "StateMachine"};
  out_templates.Clear();
  m_TemplateProperties.Clear();
  m_TemplateProperties.SetCount(XII_ARRAY_SIZE(names));
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(names); ++i)
  {
    auto& setting = m_TemplateProperties[i];
    setting.m_sPropertyName.Assign("Kind");
    setting.m_Value   = static_cast<xiiInt64>(i);
    auto& entry       = out_templates.ExpandAndGetRef();
    entry.m_pType     = xiiGetStaticRTTI<xiiAnimationGraphEditorNode>();
    entry.m_sTypeName = names[i];
    entry.m_sCategory.Assign("Animation");
    entry.m_PropertyValues = xiiArrayPtr<const xiiNodePropertyValue>(&setting, 1);
  }
}

bool xiiAnimationGraphNodeManager::InternalIsNode(const xiiDocumentObject* object) const
{
  return object->GetType()->IsDerivedFrom<xiiAnimationGraphEditorNode>();
}

void xiiAnimationGraphNodeManager::InternalCreatePins(const xiiDocumentObject* object, NodeInternal& node)
{
  if (!InternalIsNode(object))
    return;
  xiiAnimationGraphEditorNode settings;
  xiiToolsSerializationUtils::CopyProperties(object, this, &settings, xiiGetStaticRTTI<xiiAnimationGraphEditorNode>());
  const auto       color = xiiColorScheme::DarkUI(xiiColorScheme::Blue);
  xiiStringBuilder name;
  for (xiiUInt32 i = 0; i < settings.GetInputCount(); ++i)
  {
    settings.GetInputName(i, name);
    node.m_Inputs.PushBack(XII_DEFAULT_NEW(xiiPin, xiiPin::Type::Input, name, color, object));
  }
  if (settings.GetRuntimeType() != xiiAnimGraphNodeType::Output && settings.GetRuntimeType() != xiiAnimGraphNodeType::Invalid)
    node.m_Outputs.PushBack(XII_DEFAULT_NEW(xiiPin, xiiPin::Type::Output, "Pose", color, object));
}

void xiiAnimationGraphNodeManager::GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& types) const
{
  types.Clear();
  types.PushBack(xiiGetStaticRTTI<xiiAnimationGraphEditorNode>());
}

xiiStatus xiiAnimationGraphNodeManager::InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& result) const
{
  result = CanConnectResult::ConnectNever;
  if (source.GetType() != xiiPin::Type::Output || target.GetType() != xiiPin::Type::Input)
    return xiiStatus("Pose connections require an output and an input pin.");
  if (WouldConnectionCreateCircle(source, target))
    return xiiStatus("Animation graph pose dependencies must be acyclic.");
  result = CanConnectResult::ConnectNto1;
  return XII_SUCCESS;
}

bool xiiAnimationGraphNodeManager::InternalIsDynamicPinProperty(const xiiDocumentObject* object, const xiiAbstractProperty* property) const
{
  const xiiStringView name = property->GetPropertyName();
  return InternalIsNode(object) && (name == "Kind" || name == "StateCount" || name == "BlendThresholds");
}
