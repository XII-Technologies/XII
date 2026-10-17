/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Core/World/World.h>
#include <Foundation/Containers/HashSet.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <SharedPluginScene/Scene/SceneInstanceSelectionPass.h>
#include <SharedPluginScene/SharedPluginScenePCH.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneInstanceSelectionPass, 1, xiiRTTIDefaultAllocator<xiiSceneInstanceSelectionPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Candidates", m_Candidates)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("SelectionOnly", m_bSelectionOnly)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("PickSelected", m_bPickSelected)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("PickTransparent", m_bPickTransparent)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Editor/Preparation") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiSceneInstanceSelectionPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!context.m_pView || !context.m_pView->GetGpuInstanceData())
    return context.ReportFailure("Candidate filtering requires world-derived GPU instances.");
  bool       enabled = m_bEnabled, selected = m_bPickSelected, transparent = m_bPickTransparent;
  const auto parameter = [&](xiiStringView name, bool& value) -> xiiResult {
    if (const auto* setting = context.m_Parameters.GetValue(name))
    {
      xiiResult result = XII_FAILURE;
      value            = setting->ConvertTo<bool>(&result);
      XII_SUCCEED_OR_RETURN(result);
    }
    return XII_SUCCESS;
  };
  XII_SUCCEED_OR_RETURN(parameter(m_bSelectionOnly ? "Editor.SelectionOverlay" : "Editor.Picking", enabled));
  enabled = enabled && m_bEnabled;
  XII_SUCCEED_OR_RETURN(parameter("Editor.PickSelected", selected));
  XII_SUCCEED_OR_RETURN(parameter("Editor.PickTransparent", transparent));
  xiiHashSet<xiiGameObjectHandle> objects;
  if (const auto* setting = context.m_Parameters.GetValue("Editor.SelectedObjects"))
  {
    if (!setting->IsA<xiiVariantArray>())
      return context.ReportFailure("SelectedObjects must be an array of UInt64 world handles.");
    for (const auto& value : setting->Get<xiiVariantArray>())
    {
      if (!value.IsA<xiiUInt64>())
        return context.ReportFailure("SelectedObjects contains a non-UInt64 world handle.");
      xiiGameObjectId id;
      id.m_Data = value.Get<xiiUInt64>();
      objects.Insert(xiiGameObjectHandle(id));
    }
  }
  struct Data
  {
    xiiRenderGraphBufferHandle m_Buffer;
    xiiDynamicArray<xiiUInt32> m_Indices;
  };
  const auto&            view          = *context.m_pView;
  const auto&            instances     = *view.GetGpuInstanceData();
  const xiiString        previousScope = context.m_Graph.GetResourceNamespace();
  const xiiStringBuilder localScope(previousScope, "/Selection/", context.m_sPassName);
  context.m_Graph.SetResourceNamespace(localScope);
  XII_SCOPE_EXIT(context.m_Graph.SetResourceNamespace(previousScope));
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      if (enabled)
        for (auto index : view.GetGpuInstanceSelection())
        {
          if (index >= instances.GetInstances().GetCount()) continue;
          const auto& record = instances.GetInstances()[index];
          if ((record.m_uiFlags & xiiGpuInstanceFlags::Enabled) == 0 || (!transparent && (record.m_uiFlags & xiiGpuInstanceFlags::Transparent) != 0)) continue;
          xiiGpuInstanceSource source;
          if (instances.Resolve(record.m_uiObjectId, source).Failed()) continue;
          const bool contains = objects.Contains(source.m_hOwnerObject);
          if (m_bSelectionOnly ? !contains : (!selected && contains)) continue;
          data.m_Indices.PushBack(index);
        }
      if (data.m_Indices.IsEmpty()) data.m_Indices.PushBack(xiiInvalidIndex);
      xiiGALBufferCreationDescription description;
      description.m_uiSize = xiiUInt64(data.m_Indices.GetCount()) * sizeof(xiiUInt32);
      description.m_uiElementByteStride = sizeof(xiiUInt32);
      description.m_Mode = xiiGALBufferMode::Structured;
      description.m_BindFlags = xiiGALBindFlags::ShaderResource;
      data.m_Buffer = builder.WriteBuffer("Filtered World Candidates", description, xiiGALResourceStateFlags::CopyDestination); }, [](const Data& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_Buffer), 0,
                                                                                                                                                                                                                                                                   xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(data.m_Indices.GetData()), data.m_Indices.GetCount() * sizeof(xiiUInt32))); });
  return context.SetOutput(m_Candidates, emitted.first->m_Buffer);
}
XII_STATICLINK_FILE(SharedPluginScene, SharedPluginScene_Scene_Implementation_SceneInstanceSelectionPass);
