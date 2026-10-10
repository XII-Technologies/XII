/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/Graph/GraphAlgorithms.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>

xiiResult xiiRenderPipeline::Compile(const xiiRenderPipelineResourceDescriptor& descriptor, xiiSharedPtr<const xiiRenderPipeline>& out_pipeline, xiiStringBuilder* out_error)
{
  const auto fail = [&](xiiStringView message) {
    if (out_error) out_error->Set(message);
    return XII_FAILURE;
  };
  if (out_error) out_error->Clear();
  if (descriptor.m_Passes.GetCount() > 4096 || descriptor.m_Connections.GetCount() > 65536) return fail("Pipeline exceeds definition limits.");
  xiiSharedPtr<xiiRenderPipeline> result = XII_DEFAULT_NEW(xiiRenderPipeline);
  result->m_Descriptor                   = descriptor;
  XII_SUCCEED_OR_RETURN(result->m_Topology.Reserve(descriptor.m_Passes.GetCount(), descriptor.m_Connections.GetCount()));
  xiiDynamicArray<xiiGraphVertexHandle> vertices;
  xiiHashTable<xiiString, bool>         names;
  for (const auto& definition : descriptor.m_Passes)
  {
    if (definition.m_sName.IsEmpty() || names.Contains(definition.m_sName)) return fail("Pass names must be nonempty and unique.");
    names.Insert(definition.m_sName, true);
    const auto* type = xiiRTTI::FindTypeByName(definition.m_sType);
    if (!type || !type->IsDerivedFrom<xiiRenderPipelinePass>() || !type->GetAllocator()->CanAllocate()) return fail("Unknown or abstract render pipeline pass type.");
    xiiRenderPipelinePass* prototype = type->GetAllocator()->Allocate<xiiRenderPipelinePass>();
    XII_SCOPE_EXIT(type->GetAllocator()->Deallocate(prototype));
    if (prototype->Configure(definition.m_Properties).Failed()) return fail("Invalid pass configuration.");
    auto& pass             = result->m_Passes.ExpandAndGetRef();
    pass.m_pType           = type;
    pass.m_uiBindingOffset = result->m_uiBindingCount;
    prototype->GetPins(pass.m_Pins);
    if (pass.m_Pins.GetCount() > 256) return fail("Pass exceeds pin limit.");
    for (xiiUInt32 i = 0; i < pass.m_Pins.GetCount(); ++i)
    {
      const auto& pin = pass.m_Pins[i];
      if (pin.m_sName.IsEmpty()) return fail("Pin names must be nonempty.");
      for (xiiUInt32 j = 0; j < i; ++j)
        if (pass.m_Pins[j].m_bInput == pin.m_bInput && pass.m_Pins[j].m_sName == pin.m_sName) return fail("Duplicate pin name.");
    }
    result->m_uiBindingCount += pass.m_Pins.GetCount();
    xiiGraphVertexHandle vertex;
    XII_SUCCEED_OR_RETURN(result->m_Topology.TryAddVertex(vertex));
    vertices.PushBack(vertex);
  }
  xiiDynamicArray<xiiUInt8> connected;
  connected.SetCount(result->m_uiBindingCount, 0);
  for (const auto& connection : descriptor.m_Connections)
  {
    if (connection.m_uiSource >= vertices.GetCount() || connection.m_uiTarget >= vertices.GetCount()) return fail("Invalid pass endpoint.");
    const auto& source = result->m_Passes[connection.m_uiSource];
    const auto& target = result->m_Passes[connection.m_uiTarget];
    xiiUInt32   output = xiiInvalidIndex, input = xiiInvalidIndex;
    for (xiiUInt32 i = 0; i < source.m_Pins.GetCount(); ++i)
      if (!source.m_Pins[i].m_bInput && source.m_Pins[i].m_sName == connection.m_sOutput) output = i;
    for (xiiUInt32 i = 0; i < target.m_Pins.GetCount(); ++i)
      if (target.m_Pins[i].m_bInput && target.m_Pins[i].m_sName == connection.m_sInput) input = i;
    if (input == xiiInvalidIndex || output == xiiInvalidIndex) return fail("Connection references an unknown pin or incorrect direction.");
    const auto& a = source.m_Pins[output];
    const auto& b = target.m_Pins[input];
    if (a.m_Type != b.m_Type || (a.m_Format != xiiGALResourceFormat::Unknown && b.m_Format != xiiGALResourceFormat::Unknown && a.m_Format != b.m_Format)) return fail("Incompatible resource pin types or texture formats.");
    const auto targetBinding = target.m_uiBindingOffset + input;
    if (connected[targetBinding] && b.m_Type != xiiRenderPipelinePin::Order) return fail("An input resource pin has multiple producers.");
    connected[targetBinding] = 1;
    result->m_Connections.PushBack({source.m_uiBindingOffset + output, targetBinding});
    xiiGraphEdgeHandle edge;
    XII_SUCCEED_OR_RETURN(result->m_Topology.TryAddEdge(vertices[connection.m_uiSource], vertices[connection.m_uiTarget], edge));
  }
  for (const auto& pass : result->m_Passes)
    for (xiiUInt32 i = 0; i < pass.m_Pins.GetCount(); ++i)
      if (pass.m_Pins[i].m_bInput && !pass.m_Pins[i].m_bOptional && !connected[pass.m_uiBindingOffset + i]) return fail("A required input pin is disconnected.");
  xiiDynamicArray<xiiGraphVertexHandle> order;
  if (xiiGraphAlgorithms::TopologicalSort(result->m_Topology, order).Failed()) return fail("Render pipeline contains a dependency cycle.");
  for (auto vertex : order) result->m_Order.PushBack(result->m_Topology.GetVertexIndex(vertex));
  result->m_Connections.Sort([](const Connection& a, const Connection& b) { return a.m_uiTargetBinding < b.m_uiTargetBinding; });
  xiiUInt32 offset = 0;
  for (auto& pass : result->m_Passes)
  {
    pass.m_uiConnectionOffset = offset;
    while (offset < result->m_Connections.GetCount() && result->m_Connections[offset].m_uiTargetBinding < pass.m_uiBindingOffset + pass.m_Pins.GetCount()) ++offset;
    pass.m_uiConnectionCount = offset - pass.m_uiConnectionOffset;
  }
  out_pipeline = result;
  return XII_SUCCESS;
}
