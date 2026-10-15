/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/Graph/GraphAlgorithms.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>

xiiRenderPipeline::xiiRenderPipeline() :
  m_TopologyAllocator("Render Pipeline Topology", xiiDefaultAllocatorWrapper::GetAllocator()), m_Topology(&m_TopologyAllocator)
{
}

xiiResult xiiRenderPipeline::Compile(const xiiRenderPipelineResourceDescriptor& descriptor, xiiSharedPtr<const xiiRenderPipeline>& out_pipeline, xiiStringBuilder* out_error)
{
  return Compile(descriptor, {}, out_pipeline, out_error);
}

xiiResult xiiRenderPipeline::Compile(const xiiRenderPipelineResourceDescriptor& authored, xiiGraphComposition::Resolver externalResolver, xiiSharedPtr<const xiiRenderPipeline>& out_pipeline, xiiStringBuilder* out_error)
{
  xiiRenderPipelineResourceDescriptor descriptor;
  xiiUInt32                           diagnosticPass = xiiInvalidIndex;
  const auto                          fail           = [&](xiiStringView message) {
    if (out_error)
    {
      out_error->Set(message);
      if (diagnosticPass < descriptor.m_Passes.GetCount())
        out_error->Append(" Pass/instance: ", descriptor.m_Passes[diagnosticPass].m_sName);
    }
    return XII_FAILURE;
  };
  if (out_error)
    out_error->Clear();
  if (authored.m_Passes.GetCount() > 4096 || authored.m_Connections.GetCount() > 65536 || authored.m_Definitions.GetCount() > 4096)
    return fail("Pipeline exceeds definition limits.");
  xiiGraphDefinition root;
  if (authored.MakeGraphDefinition(root).Failed())
    return fail("Invalid pipeline document identities or topology.");
  xiiHashTable<xiiUuid, const xiiGraphDefinition*> definitions;
  definitions.Insert(root.m_Id, &root);
  for (const auto& definition : authored.m_Definitions)
  {
    if (definitions.Contains(definition.m_Id))
      return fail("Duplicate graph definition identity.");
    definitions.Insert(definition.m_Id, &definition);
  }
  const auto resolver = [&](const xiiUuid& id) -> const xiiGraphDefinition* {
    const xiiGraphDefinition* definition = nullptr;
    if (definitions.TryGetValue(id, definition))
      return definition;
    return externalResolver.IsValid() ? externalResolver(id) : nullptr;
  };
  xiiGraphExpansion           expanded;
  xiiGraphComposition::Limits limits;
  limits.m_uiNodes = 4096;
  limits.m_uiEdges = 65536;
  XII_SUCCEED_OR_RETURN(xiiGraphComposition::Expand(root, resolver, limits, expanded, out_error));
  descriptor.m_Id         = authored.m_Id;
  descriptor.m_Parameters = authored.m_Parameters;
  for (const auto& node : expanded.m_Nodes)
  {
    xiiStringBuilder name, id;
    for (const auto& instance : node.m_Source.m_InstancePath)
    {
      xiiConversionUtils::ToString(instance, id);
      name.Append(id, "/");
    }
    name.Append(node.m_sName);
    const auto index                = descriptor.AddPass(name, node.m_sType, node.m_Properties);
    descriptor.m_Passes[index].m_Id = node.m_Source.m_Element;
    for (const auto& instance : node.m_Source.m_InstancePath)
      descriptor.m_Passes[index].m_Id.HashCombine(instance);
  }
  for (const auto& edge : expanded.m_Edges)
  {
    auto& connection      = descriptor.m_Connections.ExpandAndGetRef();
    connection.m_uiSource = expanded.m_Graph.GetVertexIndex(expanded.m_Graph.GetSource(edge.m_hEdge));
    connection.m_uiTarget = expanded.m_Graph.GetVertexIndex(expanded.m_Graph.GetTarget(edge.m_hEdge));
    connection.m_sOutput  = edge.m_sSourcePort;
    connection.m_sInput   = edge.m_sTargetPort;
    connection.m_Id       = edge.m_Source.m_Element;
    for (const auto& instance : edge.m_Source.m_InstancePath)
      connection.m_Id.HashCombine(instance);
    connection.m_Id.HashCombine(descriptor.m_Passes[connection.m_uiTarget].m_Id);
    connection.m_Id.HashCombine(xiiUuid::MakeStableUuidFromString(connection.m_sInput));
  }
  xiiSharedPtr<xiiRenderPipeline> result = XII_DEFAULT_NEW(xiiRenderPipeline);
  result->m_Descriptor                   = descriptor;
  result->m_Dependencies                 = expanded.m_Dependencies;
  for (const auto& node : expanded.m_Nodes)
  {
    result->m_Provenance.PushBack(node.m_Source);
    result->m_PassParameters.PushBack(node.m_Parameters);
    xiiStringBuilder scope, id;
    for (const auto& instance : node.m_Source.m_InstancePath)
    {
      xiiConversionUtils::ToString(instance, id);
      if (!scope.IsEmpty())
        scope.Append("/");
      scope.Append(id);
    }
    result->m_ResourceScopes.PushBack(scope);
  }
  XII_SUCCEED_OR_RETURN(result->m_Topology.Reserve(descriptor.m_Passes.GetCount(), descriptor.m_Connections.GetCount()));
  xiiDynamicArray<xiiGraphVertexHandle>   vertices;
  xiiDynamicArray<xiiRenderPipelinePass*> prototypes;
  XII_SCOPE_EXIT(for (auto* prototype : prototypes) prototype->GetDynamicRTTI()->GetAllocator()->Deallocate(prototype));
  xiiHashTable<xiiString, bool> names;
  for (const auto& definition : descriptor.m_Passes)
  {
    diagnosticPass = result->m_Passes.GetCount();
    if (definition.m_sName.IsEmpty() || names.Contains(definition.m_sName))
      return fail("Pass names must be nonempty and unique.");
    names.Insert(definition.m_sName, true);
    const auto* type = xiiRTTI::FindTypeByName(definition.m_sType);
    if (!type || !type->IsDerivedFrom<xiiRenderPipelinePass>() || !type->GetAllocator()->CanAllocate())
      return fail("Unknown or abstract render pipeline pass type.");
    xiiRenderPipelinePass* prototype = type->GetAllocator()->Allocate<xiiRenderPipelinePass>();
    prototypes.PushBack(prototype);
    if (prototype->Configure(definition.m_Properties).Failed())
      return fail("Invalid pass configuration.");
    auto& pass             = result->m_Passes.ExpandAndGetRef();
    pass.m_pType           = type;
    pass.m_uiBindingOffset = result->m_uiBindingCount;
    prototype->GetPins(pass.m_Pins);
    if (pass.m_Pins.GetCount() > 256)
      return fail("Pass exceeds pin limit.");
    for (xiiUInt32 i = 0; i < pass.m_Pins.GetCount(); ++i)
    {
      const auto& pin = pass.m_Pins[i];
      if (pin.m_sName.IsEmpty())
        return fail("Pin names must be nonempty.");
      if (pin.m_Type > xiiRenderPipelinePin::Order || (pin.m_Type != xiiRenderPipelinePin::Texture && pin.m_Format != xiiGALResourceFormat::Unknown))
        return fail("Invalid pin resource schema.");
      for (xiiUInt32 j = 0; j < i; ++j)
        if (pass.m_Pins[j].m_bInput == pin.m_bInput && pass.m_Pins[j].m_sName == pin.m_sName)
          return fail("Duplicate pin name.");
    }
    result->m_uiBindingCount += pass.m_Pins.GetCount();
    xiiGraphVertexHandle vertex;
    XII_SUCCEED_OR_RETURN(result->m_Topology.TryAddVertex(vertex));
    vertices.PushBack(vertex);
  }
  xiiDynamicArray<xiiUInt8> connected;
  connected.SetCount(result->m_uiBindingCount, 0);
  for (const auto& boundary : expanded.m_Boundary)
  {
    diagnosticPass = expanded.m_Graph.GetVertexIndex(boundary.m_hVertex);
    bool found     = false;
    for (const auto& pin : result->m_Passes[diagnosticPass].m_Pins)
      found |= pin.m_sName == boundary.m_sPort && pin.m_bInput == boundary.m_bInput;
    if (!found)
      return fail("Subgraph interface references an unknown reflected pin or direction.");
  }
  for (const auto& connection : descriptor.m_Connections)
  {
    diagnosticPass = connection.m_uiTarget;
    if (connection.m_uiSource >= vertices.GetCount() || connection.m_uiTarget >= vertices.GetCount())
      return fail("Invalid pass endpoint.");
    const auto& source = result->m_Passes[connection.m_uiSource];
    const auto& target = result->m_Passes[connection.m_uiTarget];
    xiiUInt32   output = xiiInvalidIndex, input = xiiInvalidIndex;
    for (xiiUInt32 i = 0; i < source.m_Pins.GetCount(); ++i)
      if (!source.m_Pins[i].m_bInput && source.m_Pins[i].m_sName == connection.m_sOutput)
        output = i;
    for (xiiUInt32 i = 0; i < target.m_Pins.GetCount(); ++i)
      if (target.m_Pins[i].m_bInput && target.m_Pins[i].m_sName == connection.m_sInput)
        input = i;
    if (input == xiiInvalidIndex || output == xiiInvalidIndex)
      return fail("Connection references an unknown pin or incorrect direction.");
    const auto& a = source.m_Pins[output];
    const auto& b = target.m_Pins[input];
    if (a.m_Type != b.m_Type || (a.m_Format != xiiGALResourceFormat::Unknown && b.m_Format != xiiGALResourceFormat::Unknown && a.m_Format != b.m_Format))
      return fail("Incompatible resource pin types or texture formats.");
    const auto targetBinding = target.m_uiBindingOffset + input;
    if (connected[targetBinding] && b.m_Type != xiiRenderPipelinePin::Order)
      return fail("An input resource pin has multiple producers.");
    connected[targetBinding] = 1;
    result->m_Connections.PushBack({source.m_uiBindingOffset + output, targetBinding});
    xiiGraphEdgeHandle edge;
    XII_SUCCEED_OR_RETURN(result->m_Topology.TryAddEdge(vertices[connection.m_uiSource], vertices[connection.m_uiTarget], edge));
  }
  diagnosticPass = 0;
  for (const auto& pass : result->m_Passes)
  {
    for (xiiUInt32 i = 0; i < pass.m_Pins.GetCount(); ++i)
      if (pass.m_Pins[i].m_bInput && !pass.m_Pins[i].m_bOptional && !connected[pass.m_uiBindingOffset + i])
        return fail("A required input pin is disconnected.");
    ++diagnosticPass;
  }
  xiiDynamicArray<xiiGraphVertexHandle> order;
  if (xiiGraphAlgorithms::TopologicalSort(result->m_Topology, order).Failed())
  {
    diagnosticPass = xiiInvalidIndex;
    fail("Render pipeline contains an intra-frame dependency cycle. Use explicit pass-owned previous-frame resources for temporal history.");
    if (out_error)
      for (xiiUInt32 i = 0; i < vertices.GetCount(); ++i)
        if (!order.Contains(vertices[i]))
          out_error->Append(" ", descriptor.m_Passes[i].m_sName);
    return XII_FAILURE;
  }
  for (auto vertex : order)
    result->m_Order.PushBack(result->m_Topology.GetVertexIndex(vertex));
  for (xiiUInt32 i = 0; i < prototypes.GetCount(); ++i)
  {
    if (prototypes[i]->ValidatePipeline(descriptor, result->m_Order, i, out_error).Failed())
    {
      if (out_error)
        out_error->Append(" Pass/instance: ", descriptor.m_Passes[i].m_sName);
      return XII_FAILURE;
    }
  }
  result->m_Connections.Sort([](const Connection& a, const Connection& b) { return a.m_uiTargetBinding < b.m_uiTargetBinding; });
  xiiUInt32 offset = 0;
  for (auto& pass : result->m_Passes)
  {
    pass.m_uiConnectionOffset = offset;
    while (offset < result->m_Connections.GetCount() && result->m_Connections[offset].m_uiTargetBinding < pass.m_uiBindingOffset + pass.m_Pins.GetCount())
      ++offset;
    pass.m_uiConnectionCount = offset - pass.m_uiConnectionOffset;
  }
  out_pipeline = result;
  return XII_SUCCESS;
}
