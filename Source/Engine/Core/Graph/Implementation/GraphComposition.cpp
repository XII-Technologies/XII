/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>
#include <Core/Graph/GraphComposition.h>
#include <Core/Graph/GraphMappings.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>

namespace
{
  void Overlay(xiiVariantDictionary& target, const xiiVariantDictionary& source)
  {
    for (auto it = source.GetIterator(); it.IsValid(); ++it)
      target.Insert(it.Key(), it.Value());
  }

  xiiGraphDefinitionDependency Dependency(const xiiGraphDefinition& definition)
  {
    return {definition.m_Id, {definition.m_uiPresentationRevision, definition.m_uiParameterRevision, definition.m_uiResourceRevision, definition.m_uiTopologyRevision, definition.m_uiDefinitionRevision}};
  }

  struct CompositionEndpoint
  {
    xiiGraphVertexHandle m_hVertex;
    xiiString            m_sPort;
    xiiString            m_sName;
    bool                 m_bInput = true;
  };
  struct CompositionNode
  {
    xiiGraphVertexHandle                 m_hLeaf;
    xiiDynamicArray<CompositionEndpoint> m_Boundary;
  };
  struct CompositionOverride
  {
    xiiDynamicArray<xiiUuid> m_Path;
    xiiVariantDictionary     m_Properties;
    bool                     m_bApplied = false;
  };

  struct CompositionExpansion
  {
    CompositionExpansion(xiiGraphComposition::Resolver resolver, const xiiGraphComposition::Limits& limits, xiiStringBuilder* error) : m_Resolver(resolver), m_Limits(limits), m_pError(error) {}
    xiiGraphComposition::Resolver        m_Resolver;
    const xiiGraphComposition::Limits&   m_Limits;
    xiiGraphExpansion                    m_Result;
    xiiStringBuilder*                    m_pError;
    xiiDynamicArray<xiiUuid>             m_Definitions;
    xiiDynamicArray<xiiUuid>             m_Path;
    xiiDynamicArray<CompositionOverride> m_Overrides;
    xiiHashSet<xiiUuid>                  m_Dependencies;
    xiiUInt32                            m_uiInstances = 0;

    xiiResult Fail(xiiStringView message, const xiiUuid& definition, const xiiUuid& element = {})
    {
      if (m_pError)
      {
        xiiStringBuilder id;
        m_pError->Set(message, " Definition ");
        xiiConversionUtils::ToString(definition, id);
        m_pError->Append(id, ", instance path /");
        for (const auto& instance : m_Path)
        {
          xiiConversionUtils::ToString(instance, id);
          m_pError->Append(id, "/");
        }
        if (element.IsValid())
        {
          xiiConversionUtils::ToString(element, id);
          m_pError->Append(" element ", id);
        }
      }
      return XII_FAILURE;
    }

    xiiResult Resolve(const CompositionNode& node, xiiStringView port, bool input, xiiDynamicArray<CompositionEndpoint>& endpoints)
    {
      endpoints.Clear();
      if (!node.m_hLeaf.IsInvalidated())
        endpoints.PushBack({node.m_hLeaf, port, port, input});
      else
        for (const auto& endpoint : node.m_Boundary)
          if (endpoint.m_sName == port && endpoint.m_bInput == input)
            endpoints.PushBack(endpoint);
      return !endpoints.IsEmpty() && (input || endpoints.GetCount() == 1) ? XII_SUCCESS : XII_FAILURE;
    }

    xiiResult Visit(const xiiGraphDefinition& definition, const xiiVariantDictionary& inheritedParameters, xiiDynamicArray<CompositionEndpoint>& out_boundary)
    {
      if (m_Definitions.Contains(definition.m_Id))
        return Fail("Recursive graph definition reference.", definition.m_Id);
      if (m_Definitions.GetCount() >= m_Limits.m_uiDepth || ++m_uiInstances > m_Limits.m_uiInstances)
        return Fail("Graph expansion exceeds depth or instance limit.", definition.m_Id);
      xiiStringBuilder validation;
      if (definition.Validate(&validation).Failed())
        return Fail(validation, definition.m_Id);
      m_Definitions.PushBack(definition.m_Id);
      XII_SCOPE_EXIT(m_Definitions.PopBack());
      if (!m_Dependencies.Contains(definition.m_Id))
      {
        m_Dependencies.Insert(definition.m_Id);
        m_Result.m_Dependencies.PushBack(Dependency(definition));
      }
      xiiVariantDictionary parameters = definition.m_Parameters;
      Overlay(parameters, inheritedParameters);
      xiiDynamicArray<CompositionNode> nodes;
      nodes.SetCount(definition.m_Nodes.GetCount());
      for (xiiUInt32 i = 0; i < nodes.GetCount(); ++i)
      {
        const auto& authored        = definition.m_Nodes[i];
        auto        localParameters = parameters;
        Overlay(localParameters, authored.m_Parameters);
        if (authored.m_Definition.IsValid())
        {
          const auto* child = m_Resolver.IsValid() ? m_Resolver(authored.m_Definition) : nullptr;
          if (!child || child->m_Id != authored.m_Definition)
            return Fail("Missing or mismatched graph definition.", definition.m_Id, authored.m_Id);
          m_Path.PushBack(authored.m_Id);
          XII_SCOPE_EXIT(m_Path.PopBack());
          const auto overrideStart = m_Overrides.GetCount();
          for (const auto& authoredOverride : authored.m_Overrides)
          {
            auto& pending  = m_Overrides.ExpandAndGetRef();
            pending.m_Path = m_Path;
            pending.m_Path.PushBackRange(authoredOverride.m_Path);
            pending.m_Properties = authoredOverride.m_Properties;
          }
          XII_SUCCEED_OR_RETURN(Visit(*child, localParameters, nodes[i].m_Boundary));
          for (xiiUInt32 j = overrideStart; j < m_Overrides.GetCount(); ++j)
            if (!m_Overrides[j].m_bApplied)
              return Fail("Property override does not resolve to a leaf node.", definition.m_Id, authored.m_Id);
          m_Overrides.SetCount(overrideStart);
        }
        else
        {
          if (m_Result.m_Nodes.GetCount() >= m_Limits.m_uiNodes)
            return Fail("Graph expansion exceeds node limit.", definition.m_Id, authored.m_Id);
          XII_SUCCEED_OR_RETURN(m_Result.m_Graph.TryAddVertex(nodes[i].m_hLeaf));
          auto& expanded        = m_Result.m_Nodes.ExpandAndGetRef();
          expanded.m_hVertex    = nodes[i].m_hLeaf;
          expanded.m_Source     = {definition.m_Id, authored.m_Id, m_Path};
          expanded.m_sName      = authored.m_sName;
          expanded.m_sType      = authored.m_sType;
          expanded.m_Properties = authored.m_Properties;
          expanded.m_Parameters = std::move(localParameters);
          m_Path.PushBack(authored.m_Id);
          for (xiiUInt32 j = m_Overrides.GetCount(); j > 0; --j)
            if (m_Overrides[j - 1].m_Path == m_Path)
            {
              Overlay(expanded.m_Properties, m_Overrides[j - 1].m_Properties);
              m_Overrides[j - 1].m_bApplied = true;
            }
          m_Path.PopBack();
        }
      }

      xiiDynamicArray<CompositionEndpoint> sources, targets;
      for (xiiUInt32 i = 0; i < definition.m_Edges.GetCount(); ++i)
      {
        const auto& authored = definition.m_Edges[i];
        if (Resolve(nodes[definition.m_Topology.m_EdgeSources[i]], authored.m_sSourcePort, false, sources).Failed() || Resolve(nodes[definition.m_Topology.m_EdgeTargets[i]], authored.m_sTargetPort, true, targets).Failed())
          return Fail("Connection references an unknown boundary port or direction.", definition.m_Id, authored.m_Id);
        for (const auto& target : targets)
        {
          if (m_Result.m_Edges.GetCount() >= m_Limits.m_uiEdges)
            return Fail("Graph expansion exceeds edge limit.", definition.m_Id, authored.m_Id);
          xiiGraphEdgeHandle edge;
          XII_SUCCEED_OR_RETURN(m_Result.m_Graph.TryAddEdge(sources[0].m_hVertex, target.m_hVertex, edge));
          auto& expanded         = m_Result.m_Edges.ExpandAndGetRef();
          expanded.m_hEdge       = edge;
          expanded.m_Source      = {definition.m_Id, authored.m_Id, m_Path};
          expanded.m_sSourcePort = sources[0].m_sPort;
          expanded.m_sTargetPort = target.m_sPort;
          expanded.m_Properties  = authored.m_Properties;
        }
      }
      xiiHashTable<xiiUuid, xiiUInt32> indices;
      for (xiiUInt32 i = 0; i < definition.m_Nodes.GetCount(); ++i)
        indices.Insert(definition.m_Nodes[i].m_Id, i);
      for (const auto& mapping : definition.m_Boundary)
      {
        if (Resolve(nodes[indices[mapping.m_Node]], mapping.m_sPort, mapping.m_bInput, targets).Failed())
          return Fail("Interface mapping references an unknown boundary port or direction.", definition.m_Id, mapping.m_Node);
        for (auto endpoint : targets)
        {
          endpoint.m_sName = mapping.m_sName;
          out_boundary.PushBack(endpoint);
          auto& boundary     = m_Result.m_Boundary.ExpandAndGetRef();
          boundary.m_Source  = {definition.m_Id, mapping.m_Node, m_Path};
          boundary.m_sName   = mapping.m_sName;
          boundary.m_bInput  = mapping.m_bInput;
          boundary.m_hVertex = endpoint.m_hVertex;
          boundary.m_sPort   = endpoint.m_sPort;
        }
      }
      return XII_SUCCESS;
    }
  };
} // namespace

xiiResult xiiGraphDefinition::Validate(xiiStringBuilder* out_error) const
{
  const auto fail = [&](xiiStringView message) {
    if (out_error)
      out_error->Set(message);
    return XII_FAILURE;
  };
  if (!m_Id.IsValid() || m_Topology.Validate().Failed() || m_Nodes.GetCount() != m_Topology.m_uiVertexCount || m_Edges.GetCount() != m_Topology.m_EdgeSources.GetCount())
    return fail("Invalid definition identity or topology/payload counts.");
  xiiHashSet<xiiUuid> ids;
  for (const auto& node : m_Nodes)
  {
    if (!node.m_Id.IsValid() || ids.Contains(node.m_Id) || (!node.m_Definition.IsValid() && !node.m_Overrides.IsEmpty()))
      return fail("Invalid or duplicate node identity or leaf override.");
    ids.Insert(node.m_Id);
    for (xiiUInt32 i = 0; i < node.m_Overrides.GetCount(); ++i)
    {
      const auto& propertyOverride = node.m_Overrides[i];
      if (propertyOverride.m_Path.IsEmpty())
        return fail("An override must target a persistent node path.");
      for (xiiUInt32 j = 0; j < i; ++j)
        if (propertyOverride.m_Path == node.m_Overrides[j].m_Path)
          return fail("Duplicate property override target path.");
    }
  }
  xiiHashSet<xiiUuid> edgeIds;
  for (const auto& edge : m_Edges)
  {
    if (!edge.m_Id.IsValid() || edgeIds.Contains(edge.m_Id))
      return fail("Invalid or duplicate edge identity.");
    edgeIds.Insert(edge.m_Id);
  }
  for (xiiUInt32 i = 0; i < m_Boundary.GetCount(); ++i)
  {
    const auto& mapping = m_Boundary[i];
    if (mapping.m_sName.IsEmpty() || !ids.Contains(mapping.m_Node))
      return fail("Invalid interface mapping.");
    for (xiiUInt32 j = 0; j < i; ++j)
      if (mapping.m_bInput == m_Boundary[j].m_bInput && mapping.m_sName == m_Boundary[j].m_sName && (!mapping.m_bInput || (mapping.m_Node == m_Boundary[j].m_Node && mapping.m_sPort == m_Boundary[j].m_sPort)))
        return fail("Duplicate interface mapping or ambiguous output.");
  }
  if (out_error)
    out_error->Clear();
  return XII_SUCCESS;
}

xiiResult xiiGraphComposition::Expand(const xiiGraphDefinition& root, Resolver resolver, xiiGraphExpansion& out_expansion, xiiStringBuilder* out_error)
{
  return Expand(root, resolver, Limits{}, out_expansion, out_error);
}

xiiResult xiiGraphComposition::Expand(const xiiGraphDefinition& root, Resolver resolver, const Limits& limits, xiiGraphExpansion& out_expansion, xiiStringBuilder* out_error)
{
  if (out_error)
    out_error->Clear();
  CompositionExpansion                 expansion(resolver, limits, out_error);
  xiiDynamicArray<CompositionEndpoint> boundary;
  XII_SUCCEED_OR_RETURN(expansion.Visit(root, {}, boundary));
  // Preserve destination graph subscriptions, validator and limits. Publish payload only after topology succeeds.
  XII_SUCCEED_OR_RETURN(out_expansion.m_Graph.MoveFrom(std::move(expansion.m_Result.m_Graph)));
  out_expansion.m_Nodes        = std::move(expansion.m_Result.m_Nodes);
  out_expansion.m_Edges        = std::move(expansion.m_Result.m_Edges);
  out_expansion.m_Boundary     = std::move(expansion.m_Result.m_Boundary);
  out_expansion.m_Dependencies = std::move(expansion.m_Result.m_Dependencies);
  return XII_SUCCESS;
}

bool xiiGraphComposition::IsCurrent(xiiArrayPtr<const xiiGraphDefinitionDependency> dependencies, Resolver resolver, xiiUInt8 relevantChanges)
{
  if (!resolver.IsValid())
    return dependencies.IsEmpty();
  for (const auto& dependency : dependencies)
  {
    const auto* definition = resolver(dependency.m_Definition);
    if (!definition || definition->m_Id != dependency.m_Definition)
      return false;
    const auto current = Dependency(*definition);
    for (xiiUInt32 i = 0; i < 5; ++i)
      if ((relevantChanges & (1 << i)) && dependency.m_Revisions[i] != current.m_Revisions[i])
        return false;
  }
  return true;
}

xiiResult xiiGraphExpansion::Remap(const xiiGraphHandleMap& mapping)
{
  // Validate the complete map before changing any provenance records.
  xiiGraphVertexHandle vertex;
  xiiGraphEdgeHandle   edge;
  for (const auto& node : m_Nodes)
    if (!mapping.m_Vertices.TryGetValue(node.m_hVertex, vertex) || !m_Graph.IsValid(vertex))
      return XII_FAILURE;
  for (const auto& connection : m_Edges)
    if (!mapping.m_Edges.TryGetValue(connection.m_hEdge, edge) || !m_Graph.IsValid(edge))
      return XII_FAILURE;
  for (const auto& boundary : m_Boundary)
    if (!mapping.m_Vertices.TryGetValue(boundary.m_hVertex, vertex) || !m_Graph.IsValid(vertex))
      return XII_FAILURE;
  for (auto& node : m_Nodes)
    mapping.m_Vertices.TryGetValue(node.m_hVertex, node.m_hVertex);
  for (auto& connection : m_Edges)
    mapping.m_Edges.TryGetValue(connection.m_hEdge, connection.m_hEdge);
  for (auto& boundary : m_Boundary)
    mapping.m_Vertices.TryGetValue(boundary.m_hVertex, boundary.m_hVertex);
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Core, Core_Graph_Implementation_GraphComposition);
