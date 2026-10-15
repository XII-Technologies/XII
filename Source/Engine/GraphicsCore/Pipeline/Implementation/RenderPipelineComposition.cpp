/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineSubgraphPass, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineSubgraphPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Resource", m_sResource)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_RenderPipeline", xiiDependencyFlags::Transform | xiiDependencyFlags::Package)),
    XII_MAP_MEMBER_PROPERTY("Parameters", m_Parameters),
    XII_ARRAY_MEMBER_PROPERTY("Overrides", m_Overrides),
  } XII_END_PROPERTIES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineInterfacePass, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineInterfacePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_MEMBER_PROPERTY("Input", m_bInput),
    XII_MEMBER_PROPERTY("ResourceType", m_uiResourceType),
    XII_ENUM_MEMBER_PROPERTY("Format", xiiGALResourceFormat, m_Format),
  } XII_END_PROPERTIES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiRenderPipelineInterfacePass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const
{
  if (m_uiResourceType <= xiiRenderPipelinePin::Order && !m_sName.IsEmpty())
    out_pins.PushBack({"Value", static_cast<xiiRenderPipelinePin::Type>(m_uiResourceType), !m_bInput, false, m_Format});
}

void xiiRenderPipelineSubgraphPass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const
{
  if (m_sResource.IsEmpty())
    return;
  const auto                                 handle = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(m_sResource);
  xiiResourceLock<xiiRenderPipelineResource> acquired(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  const auto                                 source = acquired.IsValid() ? acquired->GetAuthoring() : xiiSharedPtr<const xiiRenderPipelineAuthoring>();
  if (!source)
    return;
  xiiRenderPipelineResourceDescriptor descriptor;
  if (xiiRenderPipelineResource::ResolveDefinitions(source->m_Descriptor, descriptor).Failed())
    return;
  xiiGraphDefinition definition;
  if (descriptor.MakeGraphDefinition(definition).Failed())
    return;
  // A wrapper applies the instance overrides while exposing the referenced interface unchanged.
  xiiRenderPipelineResourceDescriptor wrapper;
  wrapper.m_Definitions = descriptor.m_Definitions;
  wrapper.m_Definitions.PushBack(definition);
  wrapper.AddSubgraph("Instance", descriptor.m_Id);
  wrapper.m_Passes[0].m_Parameters = m_Parameters;
  wrapper.m_Passes[0].m_Overrides  = m_Overrides;
  for (const auto& mapping : descriptor.m_Interface)
  {
    bool found = false;
    for (const auto& existing : wrapper.m_Interface)
      found |= existing.m_bInput == mapping.m_bInput && existing.m_sName == mapping.m_sName;
    if (!found)
      wrapper.m_Interface.PushBack({mapping.m_sName, mapping.m_bInput, wrapper.m_Passes[0].m_Id, mapping.m_sName});
  }
  xiiGetRenderPipelineInterface(wrapper, out_pins).IgnoreResult();
}

xiiResult xiiGetRenderPipelineInterface(const xiiRenderPipelineResourceDescriptor& descriptor, xiiDynamicArray<xiiRenderPipelinePin>& out_pins, xiiStringBuilder* out_error)
{
  const auto fail = [&](xiiStringView message) {
    if (out_error)
      out_error->Set(message);
    return XII_FAILURE;
  };
  xiiRenderPipelineResourceDescriptor resolved;
  XII_SUCCEED_OR_RETURN(xiiRenderPipelineResource::ResolveDefinitions(descriptor, resolved, nullptr, out_error));
  xiiGraphDefinition root;
  XII_SUCCEED_OR_RETURN(resolved.MakeGraphDefinition(root));
  const auto resolver = [&](const xiiUuid& id) -> const xiiGraphDefinition* {
    if (id == root.m_Id)
      return &root;
    for (const auto& definition : resolved.m_Definitions)
      if (definition.m_Id == id)
        return &definition;
    return nullptr;
  };
  xiiGraphExpansion           expanded;
  xiiGraphComposition::Limits limits;
  limits.m_uiNodes = 4096;
  limits.m_uiEdges = 65536;
  XII_SUCCEED_OR_RETURN(xiiGraphComposition::Expand(root, resolver, limits, expanded, out_error));
  xiiDynamicArray<xiiRenderPipelinePin> result;
  for (const auto& boundary : expanded.m_Boundary)
  {
    if (!boundary.m_Source.m_InstancePath.IsEmpty())
      continue;
    const auto& node = expanded.m_Nodes[expanded.m_Graph.GetVertexIndex(boundary.m_hVertex)];
    const auto* type = xiiRTTI::FindTypeByName(node.m_sType);
    if (!type || !type->IsDerivedFrom<xiiRenderPipelinePass>() || !type->GetAllocator()->CanAllocate())
      return fail("Interface maps to an unknown reflected pass type.");
    if (node.m_sType == "xiiRenderPipelineSubgraphPass" || node.m_sType == "xiiRenderPipelineInterfacePass")
      return fail("Interface maps to an authoring node that was not lowered to a definition reference.");
    xiiRenderPipelinePass* pass = type->GetAllocator()->Allocate<xiiRenderPipelinePass>();
    XII_SCOPE_EXIT(type->GetAllocator()->Deallocate(pass));
    if (pass->Configure(node.m_Properties).Failed())
      return fail("Invalid interface pass configuration.");
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    pass->GetPins(pins);
    bool found = false;
    for (auto pin : pins)
    {
      if (pin.m_sName != boundary.m_sPort || pin.m_bInput != boundary.m_bInput)
        continue;
      pin.m_sName   = boundary.m_sName;
      bool existing = false;
      for (auto& external : result)
      {
        if (external.m_sName != pin.m_sName || external.m_bInput != pin.m_bInput)
          continue;
        if (external.m_Type != pin.m_Type || (external.m_Format != xiiGALResourceFormat::Unknown && pin.m_Format != xiiGALResourceFormat::Unknown && external.m_Format != pin.m_Format))
          return fail("Interface fan-out maps incompatible reflected pin types.");
        if (external.m_Format == xiiGALResourceFormat::Unknown)
          external.m_Format = pin.m_Format;
        external.m_bOptional &= pin.m_bOptional;
        existing = true;
      }
      if (!existing)
        result.PushBack(pin);
      found = true;
    }
    if (!found)
      return fail("Interface maps an unknown reflected pin or direction.");
  }
  out_pins = std::move(result);
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderPipelineComposition);
