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

xiiUInt64 xiiRenderPipeline::GetHeapMemoryUsage() const
{
  xiiUInt64 size = m_TopologyAllocator.GetStats().m_uiAllocationSize + m_Passes.GetHeapMemoryUsage() + m_Connections.GetHeapMemoryUsage() + m_Order.GetHeapMemoryUsage();
  size += m_Descriptor.m_Passes.GetHeapMemoryUsage() + m_Descriptor.m_Connections.GetHeapMemoryUsage() + DictionaryMemory(m_Descriptor.m_Parameters);
  for (const auto& definition : m_Descriptor.m_Passes)
    size += definition.m_sName.GetHeapMemoryUsage() + definition.m_sType.GetHeapMemoryUsage() + DictionaryMemory(definition.m_Properties);
  for (const auto& connection : m_Descriptor.m_Connections)
    size += connection.m_sInput.GetHeapMemoryUsage() + connection.m_sOutput.GetHeapMemoryUsage();
  for (const auto& pass : m_Passes)
  {
    size += pass.m_Pins.GetHeapMemoryUsage();
    for (const auto& pin : pass.m_Pins)
      size += pin.m_sName.GetHeapMemoryUsage();
  }
  return size;
}
