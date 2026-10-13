/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Strings/StringBuilder.h>
#include <SharedPluginAssets/AnimationGraphAsset/AnimationGraphAssetCompiler.h>
#include <SharedPluginAssets/AnimationGraphAsset/AnimationGraphNode.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

xiiResult xiiAnimationGraphAssetCompiler::CompileDocument(const xiiDocumentNodeManager& document, xiiArrayPtr<const xiiAnimationGraphEditorTransition> transitions,
                                                          xiiAnimGraphResourceDescriptor& out_descriptor, xiiStringBuilder* out_pError)
{
  out_descriptor.Clear();
  if (out_pError != nullptr)
    out_pError->Clear();
  auto fail = [&](const auto& message) -> xiiResult {
    if (out_pError != nullptr)
      out_pError->SetFormat(message);
    return XII_FAILURE;
  };
  const auto*                                       manager = &document;
  xiiDynamicArray<const xiiDocumentObject*>         objects;
  xiiHashTable<const xiiDocumentObject*, xiiUInt16> indices;
  for (const auto* object : manager->GetRootObject()->GetChildren())
  {
    if (!manager->IsNode(object))
      continue;
    if (objects.GetCount() == xiiMath::MaxValue<xiiUInt16>())
      return fail("Animation graph exceeds 65535 nodes.");
    objects.PushBack(object);
  }
  // Document child enumeration can change after remove/undo. UUID order keeps
  // native indices, transition references and runtime hashes stable.
  objects.Sort([](const xiiDocumentObject* a, const xiiDocumentObject* b) { return a->GetGuid() < b->GetGuid(); });
  for (xiiUInt32 i = 0; i < objects.GetCount(); ++i)
    indices.Insert(objects[i], static_cast<xiiUInt16>(i));
  xiiAnimGraphResourceDescriptor     source;
  xiiHashTable<xiiString, xiiUInt16> names;
  xiiUInt32                          outputCount = 0;
  for (const auto* object : objects)
  {
    xiiAnimationGraphEditorNode settings;
    xiiToolsSerializationUtils::CopyProperties(object, manager, &settings, xiiGetStaticRTTI<xiiAnimationGraphEditorNode>());
    if (settings.m_sName.IsEmpty())
    {
      xiiStringBuilder id;
      xiiConversionUtils::ToString(object->GetGuid(), id);
      settings.m_sName = id;
    }
    if (names.Contains(settings.m_sName))
      return fail(xiiFmt("Animation node name '{}' is not unique.", settings.m_sName));
    xiiAnimGraphNode node;
    node.m_sName.Assign(settings.m_sName);
    node.m_Type = settings.GetRuntimeType();
    node.m_sParameter.Assign(settings.m_sParameter);
    node.m_fPlaybackSpeed = settings.m_fPlaybackSpeed;
    node.m_fWeight        = settings.m_fWeight;
    node.m_uiTargetJoint  = settings.m_uiTargetJoint;
    if (node.m_Type == xiiAnimGraphNodeType::Clip && !settings.m_sClip.IsEmpty())
      node.m_hClip = xiiResourceManager::LoadResource<xiiAnimationClipResource>(settings.m_sClip);
    if (node.m_Type == xiiAnimGraphNodeType::Blend1D)
      for (const auto threshold : settings.m_BlendThresholds)
        node.m_InputThresholds.PushBack(threshold);
    const auto pins = manager->GetInputPins(object);
    if (pins.GetCount() != settings.GetInputCount())
      return fail("Animation graph pins do not match their node settings.");
    for (const auto& pin : pins)
    {
      const auto connections = manager->GetConnections(*pin);
      if (connections.GetCount() != 1)
        return fail(xiiFmt("Animation node '{}' input '{}' requires exactly one pose connection.", settings.m_sName, pin->GetName()));
      xiiUInt16 sourceIndex;
      if (!indices.TryGetValue(connections[0]->GetSourcePin().GetParent(), sourceIndex))
        return fail("Animation connection references a missing node.");
      node.m_Inputs.PushBack(sourceIndex);
    }
    const auto index = source.AddNode(node);
    names.Insert(settings.m_sName, index);
    if (node.m_Type == xiiAnimGraphNodeType::Output)
    {
      ++outputCount;
      source.SetOutputNode(index);
    }
  }
  if (outputCount != 1)
    return fail("Animation graph requires exactly one Output node.");
  for (const auto& authored : transitions)
  {
    xiiUInt16 from, to;
    if (!names.TryGetValue(authored.m_sFromNode, from) || !names.TryGetValue(authored.m_sToNode, to))
      return fail("Animation transition references a missing node name.");
    source.AddTransition(from, to, authored.m_sConditionParameter, authored.m_fConditionThreshold, authored.m_BlendDuration);
  }
  return Compile(source, out_descriptor, out_pError);
}
