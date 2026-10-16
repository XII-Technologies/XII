/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>

namespace
{
  xiiUInt64 VariantMemory(const xiiVariant& value);
  xiiUInt64 DictionaryMemory(const xiiVariantDictionary& dictionary)
  {
    xiiUInt64 size = dictionary.GetHeapMemoryUsage();
    for (const auto& entry : dictionary)
      size += entry.Key().GetHeapMemoryUsage() + VariantMemory(entry.Value());
    return size;
  }
  xiiUInt64 VariantMemory(const xiiVariant& value)
  {
    switch (value.GetType())
    {
      case xiiVariantType::String:
        return sizeof(xiiString) + value.Get<xiiString>().GetHeapMemoryUsage();
      case xiiVariantType::DataBuffer:
        return sizeof(xiiDataBuffer) + value.Get<xiiDataBuffer>().GetHeapMemoryUsage();
      case xiiVariantType::VariantDictionary:
        return sizeof(xiiVariantDictionary) + DictionaryMemory(value.Get<xiiVariantDictionary>());
      case xiiVariantType::VariantArray:
      {
        const auto& array = value.Get<xiiVariantArray>();
        xiiUInt64   size  = sizeof(xiiVariantArray) + array.GetHeapMemoryUsage();
        for (const auto& element : array)
          size += VariantMemory(element);
        return size;
      }
      case xiiVariantType::TypedObject:
        return value.GetReflectedType() ? value.GetReflectedType()->GetTypeSize() : 0;
      default:
        return 0;
    }
  }
} // namespace

xiiUInt64 xiiRenderPipelineResourceDescriptor::GetHeapMemoryUsage() const
{
  const auto overrideMemory = [](const xiiDynamicArray<xiiGraphPropertyOverride>& overrides) {
    xiiUInt64 size = overrides.GetHeapMemoryUsage();
    for (const auto& propertyOverride : overrides)
      size += propertyOverride.m_Path.GetHeapMemoryUsage() + DictionaryMemory(propertyOverride.m_Properties);
    return size;
  };
  const auto boundaryMemory = [](const xiiDynamicArray<xiiGraphBoundaryMapping>& mappings) {
    xiiUInt64 size = mappings.GetHeapMemoryUsage();
    for (const auto& mapping : mappings)
      size += mapping.m_sName.GetHeapMemoryUsage() + mapping.m_sPort.GetHeapMemoryUsage();
    return size;
  };
  xiiUInt64 size = m_Passes.GetHeapMemoryUsage() + m_Connections.GetHeapMemoryUsage() + DictionaryMemory(m_Parameters) + m_Definitions.GetHeapMemoryUsage() + m_References.GetHeapMemoryUsage() + boundaryMemory(m_Interface);
  for (const auto& pass : m_Passes)
    size += pass.m_sName.GetHeapMemoryUsage() + pass.m_sType.GetHeapMemoryUsage() + pass.m_sInstanceScope.GetHeapMemoryUsage() + DictionaryMemory(pass.m_Properties) + DictionaryMemory(pass.m_Parameters) + overrideMemory(pass.m_Overrides);
  for (const auto& connection : m_Connections)
    size += connection.m_sInput.GetHeapMemoryUsage() + connection.m_sOutput.GetHeapMemoryUsage();
  for (const auto& reference : m_References)
    size += reference.m_sResource.GetHeapMemoryUsage();
  for (const auto& definition : m_Definitions)
  {
    size += definition.m_Topology.m_EdgeSources.GetHeapMemoryUsage() + definition.m_Topology.m_EdgeTargets.GetHeapMemoryUsage() + definition.m_Nodes.GetHeapMemoryUsage() + definition.m_Edges.GetHeapMemoryUsage() + DictionaryMemory(definition.m_Parameters) + boundaryMemory(definition.m_Boundary);
    for (const auto& node : definition.m_Nodes)
      size += node.m_sName.GetHeapMemoryUsage() + node.m_sType.GetHeapMemoryUsage() + DictionaryMemory(node.m_Properties) + DictionaryMemory(node.m_Parameters) + overrideMemory(node.m_Overrides);
    for (const auto& edge : definition.m_Edges)
      size += edge.m_sSourcePort.GetHeapMemoryUsage() + edge.m_sTargetPort.GetHeapMemoryUsage() + DictionaryMemory(edge.m_Properties);
  }
  return size;
}

xiiUInt64 xiiRenderPipeline::GetHeapMemoryUsage() const
{
  xiiUInt64 size = m_TopologyAllocator.GetStats().m_uiAllocationSize + m_Passes.GetHeapMemoryUsage() + m_Connections.GetHeapMemoryUsage() + m_Order.GetHeapMemoryUsage();
  size += m_Descriptor.GetHeapMemoryUsage() + m_Provenance.GetHeapMemoryUsage() + m_Dependencies.GetHeapMemoryUsage() + m_PassParameters.GetHeapMemoryUsage() + m_ResourceScopes.GetHeapMemoryUsage() + m_GroupRequirements.GetHeapMemoryUsage() + m_GroupFeatures.GetHeapMemoryUsage();
  for (const auto& source : m_Provenance)
    size += source.m_InstancePath.GetHeapMemoryUsage();
  for (const auto& parameters : m_PassParameters)
    size += DictionaryMemory(parameters);
  for (const auto& scope : m_ResourceScopes)
    size += scope.GetHeapMemoryUsage();
  for (const auto& pass : m_Passes)
  {
    size += pass.m_Pins.GetHeapMemoryUsage();
    for (const auto& pin : pass.m_Pins)
      size += pin.m_sName.GetHeapMemoryUsage() + pin.m_sDisplayName.GetHeapMemoryUsage();
  }
  return size;
}
