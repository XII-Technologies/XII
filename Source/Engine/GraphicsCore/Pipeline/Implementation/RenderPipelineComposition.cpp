/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>
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
    XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("Input")),
    XII_MEMBER_PROPERTY("Input", m_bInput)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("ResourceType", m_uiResourceType),
    XII_ENUM_MEMBER_PROPERTY("Format", xiiGALResourceFormat, m_Format),
    XII_ACCESSOR_PROPERTY("DataType", GetDataTypeName, SetDataTypeName),
  } XII_END_PROPERTIES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiStringView xiiRenderPipelineInterfacePass::GetDataTypeName() const
{
  return m_pDataType ? m_pDataType->GetTypeName() : "";
}

void xiiRenderPipelineInterfacePass::SetDataTypeName(xiiStringView sTypeName)
{
  m_pDataType = xiiRTTI::FindTypeByName(sTypeName);
}

void xiiRenderPipelineInterfacePass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const
{
  if (m_uiResourceType <= xiiRenderPipelinePin::FrameData && !m_sName.IsEmpty())
  {
    auto& pin       = out_pins.ExpandAndGetRef();
    pin.m_sName     = "Value";
    pin.m_Type      = static_cast<xiiRenderPipelinePin::Type>(m_uiResourceType);
    pin.m_bInput    = !m_bInput;
    pin.m_Format    = m_Format;
    pin.m_pDataType = pin.m_Type == xiiRenderPipelinePin::FrameData ? m_pDataType : nullptr;
  }
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

xiiResult xiiApplyRenderPipelineInterfaceContracts(const xiiGraphDefinition& root, xiiGraphComposition::Resolver resolver, const xiiGraphExpansion& expanded, xiiArrayPtr<xiiArrayPtr<xiiRenderPipelinePin>> pinSchemas, xiiStringBuilder* out_error)
{
  const auto fail = [&](xiiStringView message) {
    if (out_error)
      out_error->Set(message);
    return XII_FAILURE;
  };
  struct InterfaceConfigurations
  {
    xiiDynamicArray<xiiRenderPipelinePinConfiguration> m_Configurations;
    xiiDynamicArray<bool>                              m_Used;
  };
  xiiMap<xiiString, InterfaceConfigurations> interfaces;
  xiiDynamicArray<xiiUuid>                   instancePath;
  const auto                                 pathKey = [](xiiArrayPtr<const xiiUuid> path) {
    xiiStringBuilder key, id;
    for (const auto& element : path)
    {
      xiiConversionUtils::ToString(element, id);
      key.Append(id, "/");
    }
    return key;
  };
  const auto collectInterfaces = [&](auto&& self, const xiiGraphDefinition& definition) -> xiiResult {
    for (const auto& node : definition.m_Nodes)
    {
      if (!node.m_Definition.IsValid())
        continue;
      instancePath.PushBack(node.m_Id);
      XII_SCOPE_EXIT(instancePath.PopBack());
      if (!node.m_Properties.IsEmpty())
      {
        if (node.m_Properties.GetCount() != 1 || !node.m_Properties.Contains("PinConfigurations"))
          return fail("Unknown subgraph instance property; use explicit node overrides or pin configurations.");
        xiiRenderPipelineSubgraphPass instance;
        if (instance.Configure(node.m_Properties).Failed() || instance.m_PinConfigurations.GetCount() > 256)
          return fail("Invalid subgraph interface pin configurations.");
        auto&                 group = interfaces[pathKey(instancePath)];
        xiiHashSet<xiiString> bindings;
        for (const auto* configuration : instance.m_PinConfigurations)
        {
          if (!configuration)
            return fail("Null subgraph interface pin configuration.");
          xiiStringBuilder binding(configuration->m_Metadata.m_bInput ? "Input/" : "Output/", configuration->m_Metadata.m_sName);
          if (bindings.Contains(binding))
            return fail("Duplicate subgraph interface pin configuration.");
          bindings.Insert(binding);
          group.m_Configurations.PushBack(*configuration);
          group.m_Used.PushBack(false);
        }
      }
      const auto* child = resolver(node.m_Definition);
      if (!child || instancePath.GetCount() > 128)
        return fail("Invalid subgraph definition while applying interface contracts.");
      XII_SUCCEED_OR_RETURN(self(self, *child));
    }
    return XII_SUCCESS;
  };
  XII_SUCCEED_OR_RETURN(collectInterfaces(collectInterfaces, root));
  for (const auto& boundary : expanded.m_Boundary)
  {
    auto group = interfaces.Find(pathKey(boundary.m_Source.m_InstancePath));
    if (!group.IsValid())
      continue;
    const auto index = expanded.m_Graph.GetVertexIndex(boundary.m_hVertex);
    for (xiiUInt32 i = 0; i < group.Value().m_Configurations.GetCount(); ++i)
    {
      const auto& configuration = group.Value().m_Configurations[i];
      if (configuration.m_Metadata.m_sName != boundary.m_sName || configuration.m_Metadata.m_bInput != boundary.m_bInput)
        continue;
      bool found = false;
      for (auto& pin : pinSchemas[index])
        if (pin.m_sName == boundary.m_sPort && pin.m_bInput == boundary.m_bInput)
        {
          xiiStringBuilder error;
          if (pin.Apply(configuration, &error).Failed())
          {
            error.Append(" Interface instance: ", group.Key());
            return fail(error);
          }
          found                   = true;
          group.Value().m_Used[i] = true;
          break;
        }
      if (!found)
        return fail("Subgraph contract maps an unknown internal pin.");
    }
  }
  for (auto group = interfaces.GetIterator(); group.IsValid(); ++group)
    for (xiiUInt32 i = 0; i < group.Value().m_Used.GetCount(); ++i)
      if (!group.Value().m_Used[i])
      {
        xiiStringBuilder error;
        error.SetFormat("Unknown interface pin '{}' in instance '{}'.", group.Value().m_Configurations[i].m_Metadata.m_sName, group.Key());
        return fail(error);
      }
  return XII_SUCCESS;
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
  xiiDynamicArray<xiiRenderPipelinePin>                  result;
  xiiDynamicArray<xiiDynamicArray<xiiRenderPipelinePin>> schemas;
  schemas.SetCount(expanded.m_Nodes.GetCount());
  xiiDynamicArray<xiiArrayPtr<xiiRenderPipelinePin>> pinSchemas;
  for (xiiUInt32 index = 0; index < expanded.m_Nodes.GetCount(); ++index)
  {
    const auto& node = expanded.m_Nodes[index];
    const auto* type = xiiRTTI::FindTypeByName(node.m_sType);
    if (!type || !type->IsDerivedFrom<xiiRenderPipelinePass>() || !type->GetAllocator()->CanAllocate())
      return fail("Interface maps to an unknown reflected pass type.");
    if (type->IsDerivedFrom<xiiRenderPipelineSubgraphPass>() || type->IsDerivedFrom<xiiRenderPipelineInterfacePass>())
      return fail("Interface maps to an authoring node that was not lowered to a definition reference.");
    xiiRenderPipelinePass* pass = type->GetAllocator()->Allocate<xiiRenderPipelinePass>();
    XII_SCOPE_EXIT(type->GetAllocator()->Deallocate(pass));
    if (pass->Configure(node.m_Properties).Failed())
      return fail("Invalid interface pass configuration.");
    XII_SUCCEED_OR_RETURN(pass->GetConfiguredPins(schemas[index], out_error));
    pinSchemas.PushBack(schemas[index].GetArrayPtr());
  }
  XII_SUCCEED_OR_RETURN(xiiApplyRenderPipelineInterfaceContracts(root, resolver, expanded, pinSchemas, out_error));
  for (const auto& boundary : expanded.m_Boundary)
  {
    if (!boundary.m_Source.m_InstancePath.IsEmpty())
      continue;
    const auto& pins  = schemas[expanded.m_Graph.GetVertexIndex(boundary.m_hVertex)];
    bool        found = false;
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
        if (!external.IsCompatibleWith(pin))
          return fail("Interface fan-out maps incompatible reflected pin types.");
        if (external.m_Format == xiiGALResourceFormat::Unknown)
          external.m_Format = pin.m_Format;
        if (external.m_Constraints.m_Dimension == xiiGALResourceDimension::Undefined)
          external.m_Constraints.m_Dimension = pin.m_Constraints.m_Dimension;
        if (!external.m_Constraints.m_uiSamples)
          external.m_Constraints.m_uiSamples = pin.m_Constraints.m_uiSamples;
        if (!external.m_Constraints.m_uiWidth)
          external.m_Constraints.m_uiWidth = pin.m_Constraints.m_uiWidth;
        if (!external.m_Constraints.m_uiHeight)
          external.m_Constraints.m_uiHeight = pin.m_Constraints.m_uiHeight;
        external.m_Constraints.m_RequiredBindFlags.Add(pin.m_Constraints.m_RequiredBindFlags);
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
