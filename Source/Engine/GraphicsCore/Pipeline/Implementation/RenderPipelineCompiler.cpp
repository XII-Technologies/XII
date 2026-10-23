/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graph/GraphAlgorithms.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>

xiiRenderPipeline::xiiRenderPipeline() :
  m_TopologyAllocator("Render Pipeline Topology", xiiDefaultAllocatorWrapper::GetAllocator()), m_Topology(&m_TopologyAllocator)
{
}

xiiResult xiiRenderPipeline::Compile(const xiiRenderPipelineResourceDescriptor& descriptor, xiiSharedPtr<const xiiRenderPipeline>& out_pipeline, xiiStringBuilder* out_error)
{
  if (!descriptor.m_References.IsEmpty())
  {
    xiiRenderPipelineResourceDescriptor resolved;
    XII_SUCCEED_OR_RETURN(xiiRenderPipelineResource::ResolveDefinitions(descriptor, resolved, nullptr, out_error));
    return Compile(resolved, {}, out_pipeline, out_error);
  }
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
  XII_SUCCEED_OR_RETURN(authored.m_Settings.Validate(out_error));
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
  descriptor.m_Settings   = authored.m_Settings;
  for (const auto& node : expanded.m_Nodes)
  {
    xiiStringBuilder name, id;
    for (const auto& instance : node.m_Source.m_InstancePath)
    {
      xiiConversionUtils::ToString(instance, id);
      name.Append(id, "/");
    }
    name.Append(node.m_sName);
    const auto index                        = descriptor.AddPass(name, xiiRTTI::FindTypeByName(node.m_sType), node.m_Properties);
    descriptor.m_Passes[index].m_Id         = node.m_Source.m_Element;
    descriptor.m_Passes[index].m_Parameters = node.m_Parameters;
    xiiStringBuilder instanceScope;
    for (const auto& instance : node.m_Source.m_InstancePath)
    {
      xiiConversionUtils::ToString(instance, id);
      if (!instanceScope.IsEmpty())
        instanceScope.Append("/");
      instanceScope.Append(id);
    }
    descriptor.m_Passes[index].m_sInstanceScope = instanceScope;
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
    const auto* type = definition.m_pType;
    if (!type || !type->IsDerivedFrom<xiiRenderPipelinePass>() || !type->GetAllocator()->CanAllocate())
      return fail("Unknown or abstract render pipeline pass type.");
    if (type->IsDerivedFrom<xiiRenderPipelineSubgraphPass>() || type == xiiGetStaticRTTI<xiiRenderPipelineInterfacePass>())
      return fail("Authoring interface and subgraph nodes must be lowered to definition references before compilation.");
    xiiRenderPipelinePass* prototype = type->GetAllocator()->Allocate<xiiRenderPipelinePass>();
    prototypes.PushBack(prototype);
    if (prototype->Configure(definition.m_Properties).Failed())
      return fail("Invalid pass configuration.");
    auto& pass             = result->m_Passes.ExpandAndGetRef();
    pass.m_pType           = type;
    pass.m_uiBindingOffset = result->m_uiBindingCount;
    xiiStringBuilder pinError;
    if (prototype->GetConfiguredPins(pass.m_Pins, &pinError).Failed())
      return fail(pinError.IsEmpty() ? "Invalid authored pin configuration."_xiisv : pinError.GetView());
    result->m_ExtractionRequirements |= prototype->GetExtractionRequirements();
    result->m_RuntimeFeatures |= prototype->GetRuntimeFeatures();
    result->m_Descriptor.m_Settings.m_RequiredDeviceFeatures.Add(prototype->GetRequiredDeviceFeatures());
    if (pass.m_Pins.GetCount() > 256)
      return fail("Pass exceeds pin limit.");
    for (xiiUInt32 i = 0; i < pass.m_Pins.GetCount(); ++i)
    {
      const auto& pin = pass.m_Pins[i];
      if (pin.m_sName.IsEmpty())
        return fail("Pin names must be nonempty.");
      if (pin.m_bTemporalHistoryOutput)
      {
        if (pin.m_bInput || (pin.m_Type != xiiRenderPipelinePin::Texture && pin.m_Type != xiiRenderPipelinePin::Buffer))
          return fail("Temporal history must be a texture or buffer output pin.");
        pass.m_bHasTemporalHistory = true;
      }
      if ((pin.m_Type != xiiRenderPipelinePin::Texture && pin.m_Type != xiiRenderPipelinePin::Buffer && pin.m_Type != xiiRenderPipelinePin::AccelerationStructure && pin.m_Type != xiiRenderPipelinePin::FrameData) || (pin.m_Type != xiiRenderPipelinePin::Texture && pin.m_Format != xiiGALResourceFormat::Unknown))
        return fail("Invalid pin resource schema.");
      if ((pin.m_Type == xiiRenderPipelinePin::FrameData) != (pin.m_pDataType != nullptr) || (pin.m_pDataType && !pin.m_pDataType->IsDerivedFrom<xiiRenderPipelineFrameData>()))
        return fail("Frame-data pins require a native reflected snapshot type.");
      for (xiiUInt32 j = 0; j < i; ++j)
        if (pass.m_Pins[j].m_bInput == pin.m_bInput && pass.m_Pins[j].m_sName == pin.m_sName)
          return fail("Duplicate pin name.");
    }
    result->m_uiBindingCount += pass.m_Pins.GetCount();
    xiiGraphVertexHandle vertex;
    XII_SUCCEED_OR_RETURN(result->m_Topology.TryAddVertex(vertex));
    vertices.PushBack(vertex);
  }
  xiiDynamicArray<xiiArrayPtr<xiiRenderPipelinePin>> pinSchemas;
  for (auto& pass : result->m_Passes)
    pinSchemas.PushBack(pass.m_Pins.GetArrayPtr());
  xiiStringBuilder interfaceError;
  diagnosticPass = xiiInvalidIndex;
  if (xiiApplyRenderPipelineInterfaceContracts(root, resolver, expanded, pinSchemas, &interfaceError).Failed())
    return fail(interfaceError);
  for (xiiUInt32 index = 0; index < result->m_Passes.GetCount(); ++index)
    for (auto& pin : result->m_Passes[index].m_Pins)
    {
      if (!pin.m_SourceId.IsValid())
      {
        xiiStringBuilder key(pin.m_bInput ? "Input/" : "Output/", pin.m_sName);
        pin.m_SourceId = xiiUuid::MakeStableUuidFromString(key);
        pin.m_SourceId.HashCombine(expanded.m_Nodes[index].m_Source.m_Element);
      }
      pin.m_Id = pin.m_SourceId;
      pin.m_Id.HashCombine(descriptor.m_Id);
      pin.m_Id.HashCombine(descriptor.m_Passes[index].m_Id);
    }
  xiiHashTable<xiiString, xiiBitflags<xiiRenderExtractionCategory>> groups;
  for (xiiUInt32 i = 0; i < prototypes.GetCount(); ++i)
    groups[result->m_ResourceScopes[i]] |= prototypes[i]->GetExtractionRequirements();
  for (const auto& scope : result->m_ResourceScopes)
    result->m_GroupRequirements.PushBack(groups[scope]);
  xiiHashTable<xiiString, xiiBitflags<xiiRenderPipelineFeature>> featureGroups;
  for (xiiUInt32 i = 0; i < prototypes.GetCount(); ++i)
    featureGroups[result->m_ResourceScopes[i]] |= prototypes[i]->GetRuntimeFeatures();
  for (const auto& scope : result->m_ResourceScopes)
    result->m_GroupFeatures.PushBack(featureGroups[scope]);
  xiiDynamicArray<xiiUInt8> connected;
  connected.SetCount(result->m_uiBindingCount, 0);
  xiiHashTable<xiiString, xiiRenderPipelinePin> boundarySchemas;
  xiiHashTable<xiiString, xiiUInt32>            rootInterfaces;
  for (const auto& boundary : expanded.m_Boundary)
  {
    xiiStringBuilder boundaryKey, sourceId;
    xiiConversionUtils::ToString(boundary.m_Source.m_Definition, sourceId);
    boundaryKey.SetFormat("{}:{}:{}:", sourceId, boundary.m_Source.m_InstancePath.GetCount(), boundary.m_bInput);
    for (const auto& instance : boundary.m_Source.m_InstancePath)
    {
      xiiConversionUtils::ToString(instance, sourceId);
      boundaryKey.Append(sourceId, "/");
    }
    boundaryKey.AppendFormat("{}:{}", boundary.m_sName.GetElementCount(), boundary.m_sName);
    diagnosticPass    = expanded.m_Graph.GetVertexIndex(boundary.m_hVertex);
    bool        found = false;
    const auto& pass  = result->m_Passes[diagnosticPass];
    for (xiiUInt32 i = 0; i < pass.m_Pins.GetCount(); ++i)
      if (pass.m_Pins[i].m_sName == boundary.m_sPort && pass.m_Pins[i].m_bInput == boundary.m_bInput)
      {
        found                          = true;
        const auto&           pin      = pass.m_Pins[i];
        xiiRenderPipelinePin* existing = nullptr;
        if (boundarySchemas.TryGetValue(boundaryKey.GetView(), existing))
        {
          if (!existing->IsCompatibleWith(pin))
            return fail("Subgraph interface fan-out maps incompatible reflected resource pins.");
          if (existing->m_Format == xiiGALResourceFormat::Unknown)
            existing->m_Format = pin.m_Format;
          if (pin.m_bConstrainResolution && !existing->m_bConstrainResolution)
            existing->m_Resolution = pin.m_Resolution;
          else if (pin.m_bConstrainResolution && existing->m_Resolution != pin.m_Resolution)
            return fail("Interface fan-out maps conflicting texture resolution policies.");
          existing->m_bConstrainResolution |= pin.m_bConstrainResolution;
          if (existing->m_Constraints.m_Dimension == xiiGALResourceDimension::Undefined)
            existing->m_Constraints.m_Dimension = pin.m_Constraints.m_Dimension;
          if (!existing->m_Constraints.m_uiSamples)
            existing->m_Constraints.m_uiSamples = pin.m_Constraints.m_uiSamples;
          if (!existing->m_Constraints.m_uiWidth)
            existing->m_Constraints.m_uiWidth = pin.m_Constraints.m_uiWidth;
          if (!existing->m_Constraints.m_uiHeight)
            existing->m_Constraints.m_uiHeight = pin.m_Constraints.m_uiHeight;
          existing->m_Constraints.m_RequiredBindFlags.Add(pin.m_Constraints.m_RequiredBindFlags);
          existing->m_bOptional &= pin.m_bOptional;
        }
        else
          boundarySchemas.Insert(boundaryKey.GetView(), pin);
        if (boundary.m_Source.m_InstancePath.IsEmpty())
        {
          xiiUInt32 external = 0;
          if (!rootInterfaces.TryGetValue(boundaryKey.GetView(), external))
          {
            external = result->m_Interface.GetCount();
            rootInterfaces.Insert(boundaryKey.GetView(), external);
            result->m_Interface.ExpandAndGetRef();
          }
          auto& externalMapping         = result->m_Interface[external];
          externalMapping.m_Pin         = boundarySchemas[boundaryKey.GetView()];
          externalMapping.m_Pin.m_sName = boundary.m_sName;
          const auto binding            = pass.m_uiBindingOffset + i;
          if (!boundary.m_bInput && !externalMapping.m_Bindings.IsEmpty() && !externalMapping.m_Bindings.Contains(binding))
            return fail("An external output maps multiple resource producers.");
          if (!externalMapping.m_Bindings.Contains(binding))
            externalMapping.m_Bindings.PushBack(binding);
          if (boundary.m_bInput)
            connected[binding] = 1;
        }
      }
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
    if (!a.IsCompatibleWith(b))
      return fail("Incompatible resource pin types, formats, samples or dimensions.");
    const auto targetBinding = target.m_uiBindingOffset + input;
    if (connected[targetBinding])
      return fail("An input resource pin has multiple producers.");
    connected[targetBinding] = 1;
    result->m_Connections.PushBack({source.m_uiBindingOffset + output, targetBinding});
    if (!a.m_bTemporalHistoryOutput)
    {
      xiiGraphEdgeHandle edge;
      XII_SUCCEED_OR_RETURN(result->m_Topology.TryAddEdge(vertices[connection.m_uiSource], vertices[connection.m_uiTarget], edge));
    }
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
