/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <Core/Graph/Graph.h>
#include <Core/Graph/GraphAlgorithms.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/Strings/StringBuilder.h>
#include <SharedPluginAssets/AnimationGraphAsset/AnimationGraphAssetCompiler.h>

xiiResult xiiAnimationGraphAssetCompiler::Compile(const xiiAnimGraphResourceDescriptor& source, xiiAnimGraphResourceDescriptor& out_descriptor,
                                                  xiiStringBuilder* out_pError)
{
  if (out_pError != nullptr)
    out_pError->Clear();
  // Aliasing would erase the source and cannot provide transactional validation.
  if (&source == &out_descriptor)
  {
    if (out_pError != nullptr)
      *out_pError = "Animation graph compilation requires distinct source and output descriptors.";
    return XII_FAILURE;
  }
  out_descriptor.Clear();
  auto fail = [&](xiiStringView message) -> xiiResult {
    if (out_pError != nullptr)
      *out_pError = message;
    return XII_FAILURE;
  };
  const auto count = source.m_Nodes.GetCount();
  if (count == 0 || count > xiiMath::MaxValue<xiiUInt16>() || source.m_uiOutputNode >= count)
    return fail("Animation graph requires nodes and a valid output, with at most 65535 nodes.");

  xiiGraph topology;
  if (topology.Reserve(count, count).Failed())
    return fail("Could not reserve animation graph topology.");
  xiiDynamicArray<xiiGraphVertexHandle> vertices;
  vertices.SetCount(count);
  xiiHashSet<xiiHashedString>                   names;
  xiiDynamicArray<xiiHybridArray<xiiUInt16, 2>> stateOwners;
  stateOwners.SetCount(count);
  for (xiiUInt32 i = 0; i < count; ++i)
    if (topology.TryAddVertex(vertices[i]).Failed())
      return fail("Could not create animation graph topology.");
  for (xiiUInt32 i = 0; i < count; ++i)
  {
    const auto& node = source.m_Nodes[i];
    if (node.m_sName.IsEmpty() || names.Insert(node.m_sName))
      return fail("Animation graph node names must be nonempty and unique.");
    if (!xiiMath::IsFinite(node.m_fPlaybackSpeed) || !xiiMath::IsFinite(node.m_fWeight) || !xiiMath::IsFinite(node.m_fThreshold))
      return fail("Animation graph node settings must be finite.");
    const auto inputs = node.m_Inputs.GetCount();
    switch (node.m_Type.GetValue())
    {
      case xiiAnimGraphNodeType::Clip:
        if (inputs != 0 || !node.m_hClip.IsValid())
          return fail("Clip nodes require a clip resource and have no pose inputs.");
        break;
      case xiiAnimGraphNodeType::Output:
        if (inputs != 1)
          return fail("Output nodes require exactly one pose input.");
        break;
      case xiiAnimGraphNodeType::Additive:
      case xiiAnimGraphNodeType::LayeredBlend:
        if (inputs != 2)
          return fail("Additive and layered blend nodes require exactly two pose inputs.");
        break;
      case xiiAnimGraphNodeType::Blend1D:
        if (inputs == 0 || node.m_InputThresholds.GetCount() != inputs || node.m_sParameter.IsEmpty())
          return fail("Blend1D nodes require a parameter and one threshold per connected pose input.");
        for (xiiUInt32 n = 0; n < inputs; ++n)
          if (!xiiMath::IsFinite(node.m_InputThresholds[n]) || (n > 0 && node.m_InputThresholds[n] <= node.m_InputThresholds[n - 1]))
            return fail("Blend1D thresholds must be finite and strictly increasing in pin order.");
        break;
      case xiiAnimGraphNodeType::StateMachine:
        if (inputs == 0)
          return fail("State machines require at least one state; the first state is initially active.");
        break;
      default:
        return fail("Animation graph node kind is not implemented by the native runtime.");
    }
    if (!node.m_InputWeights.IsEmpty() || (node.m_Type != xiiAnimGraphNodeType::Blend1D && !node.m_InputThresholds.IsEmpty()))
      return fail("Animation graph contains input settings not consumed by its node kind.");
    xiiHashSet<xiiUInt16> uniqueStates;
    for (const auto input : node.m_Inputs)
    {
      if (input >= count)
        return fail("Animation graph pose connection references a missing node.");
      if (node.m_Type == xiiAnimGraphNodeType::StateMachine)
      {
        if (uniqueStates.Insert(input))
          return fail("A state machine cannot list the same state more than once.");
        stateOwners[input].PushBack(static_cast<xiiUInt16>(i));
      }
      xiiGraphEdgeHandle edge;
      if (topology.TryAddEdge(vertices[input], vertices[i], edge).Failed())
        return fail("Could not create animation graph dependency.");
    }
  }

  xiiDynamicArray<xiiGraphVertexHandle> order;
  if (xiiGraphAlgorithms::TopologicalSort(topology, order).Failed())
    return fail("Animation graph pose dependencies contain a cycle.");
  xiiDynamicArray<xiiUInt32> depth;
  depth.SetCount(count, 1);
  for (const auto vertex : order)
  {
    const auto index = topology.GetVertexIndex(vertex);
    if (depth[index] > 128)
      return fail("Animation graph exceeds the native runtime's supported evaluation depth of 128.");
    for (auto edge = topology.GetOutgoingEdges(vertex); edge.IsValid(); edge.Next())
    {
      const auto target = topology.GetVertexIndex(topology.GetTarget(edge.GetHandle()));
      depth[target]     = xiiMath::Max(depth[target], depth[index] + 1);
    }
  }
  for (const auto& transition : source.m_Transitions)
  {
    if (transition.m_uiFromNode >= count || transition.m_uiToNode >= count ||
        !xiiMath::IsFinite(transition.m_fConditionThreshold) || !xiiMath::IsFinite(transition.m_BlendDuration.GetSeconds()) ||
        transition.m_BlendDuration.GetSeconds() < 0)
      return fail("Animation graph transition has invalid states, threshold or blend duration.");
    // Owner lists are sorted by node index during construction. Intersect them
    // without imposing exclusive ownership on reusable state subgraphs.
    const auto& from = stateOwners[transition.m_uiFromNode];
    const auto& to   = stateOwners[transition.m_uiToNode];
    xiiUInt32   a = 0, b = 0;
    while (a < from.GetCount() && b < to.GetCount() && from[a] != to[b])
    {
      if (from[a] < to[b])
        ++a;
      else
        ++b;
    }
    if (a == from.GetCount() || b == to.GetCount())
      return fail("Transition endpoints must be direct states of the same state machine.");
  }
  out_descriptor = source;
  out_descriptor.ComputeRuntimeHash();
  return XII_SUCCESS;
}
