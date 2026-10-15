/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/Graph/GraphAlgorithms.h>
#include <Core/Graph/GraphComposition.h>
#include <Core/Graph/GraphCompositionCache.h>
#include <Core/Graph/GraphMappings.h>
#include <CoreTest/CoreTestPCH.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/ReflectionSerializer.h>

namespace GraphCompositionTest
{
  xiiGraphDefinition Make(xiiInt64 id, xiiUInt32 nodes)
  {
    xiiGraphDefinition definition;
    definition.m_Id                       = xiiUuid::MakeStableUuidFromInt(id);
    definition.m_Topology.m_uiVertexCount = nodes;
    for (xiiUInt32 i = 0; i < nodes; ++i)
      definition.m_Nodes.ExpandAndGetRef().m_Id = xiiUuid::MakeStableUuidFromInt(id * 100 + i);
    return definition;
  }
  void Link(xiiGraphDefinition& definition, xiiUInt32 source, xiiUInt32 target)
  {
    definition.m_Topology.m_EdgeSources.PushBack(source);
    definition.m_Topology.m_EdgeTargets.PushBack(target);
    auto& edge         = definition.m_Edges.ExpandAndGetRef();
    edge.m_Id          = xiiUuid::MakeStableUuidFromInt(10000 + definition.m_Edges.GetCount());
    edge.m_sSourcePort = "Output";
    edge.m_sTargetPort = "Input";
  }
} // namespace GraphCompositionTest

XII_CREATE_SIMPLE_TEST(Graph, Composition)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Selective immutable composition cache invalidation")
  {
    auto leaf = GraphCompositionTest::Make(6, 1);
    auto unrelated = GraphCompositionTest::Make(7, 1);
    auto root = GraphCompositionTest::Make(8, 2);
    for (auto& node : root.m_Nodes)
      node.m_Definition = leaf.m_Id;
    const auto resolver = [&](const xiiUuid& id) -> const xiiGraphDefinition* {
      if (id == leaf.m_Id) return &leaf;
      if (id == unrelated.m_Id) return &unrelated;
      return nullptr;
    };
    xiiGraphCompositionCache cache;
    xiiSharedPtr<const xiiGraphCompiledComposition> first, current;
    bool reused = true;
    XII_TEST_RESULT(cache.Compile(root, resolver, first, &reused));
    XII_TEST_BOOL(!reused);
    ++leaf.m_uiPresentationRevision;
    ++unrelated.m_uiTopologyRevision;
    XII_TEST_RESULT(cache.Compile(root, resolver, current, &reused));
    XII_TEST_BOOL(reused && first == current);
    leaf.m_Nodes[0].m_Properties.Insert("Runtime", 9U);
    ++leaf.m_uiParameterRevision;
    XII_TEST_RESULT(cache.Compile(root, resolver, current, &reused));
    XII_TEST_BOOL(!reused && first != current);
    XII_TEST_BOOL(!first->m_Expansion.m_Nodes[0].m_Properties.Contains("Runtime"));
    leaf.m_Nodes[0].m_Definition = root.m_Id;
    ++leaf.m_uiDefinitionRevision;
    const auto saved = current;
    XII_TEST_BOOL(cache.Compile(root, resolver, current).Failed());
    XII_TEST_BOOL(saved == current);
    cache.Clear();
    XII_TEST_INT(first->m_Expansion.m_Graph.GetVertexCount(), 2);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Nested shared definitions, boundary fan-out, overrides and compaction")
  {
    auto leaf = GraphCompositionTest::Make(1, 2);
    GraphCompositionTest::Link(leaf, 0, 1);
    leaf.m_Boundary.PushBack({"Input", true, leaf.m_Nodes[0].m_Id, "Input"});
    leaf.m_Boundary.PushBack({"Input", true, leaf.m_Nodes[1].m_Id, "Extra"});
    leaf.m_Boundary.PushBack({"Output", false, leaf.m_Nodes[1].m_Id, "Output"});
    leaf.m_Nodes[0].m_Properties.Insert("Value", 1U);
    auto nested                    = GraphCompositionTest::Make(2, 1);
    nested.m_Nodes[0].m_Definition = leaf.m_Id;
    nested.m_Boundary.PushBack({"Input", true, nested.m_Nodes[0].m_Id, "Input"});
    nested.m_Boundary.PushBack({"Output", false, nested.m_Nodes[0].m_Id, "Output"});
    auto root                    = GraphCompositionTest::Make(3, 2);
    root.m_Nodes[0].m_Definition = nested.m_Id;
    root.m_Nodes[1].m_Definition = nested.m_Id;
    GraphCompositionTest::Link(root, 0, 1);
    root.m_Parameters.Insert("Shared", 7U);
    root.m_Nodes[1].m_Parameters.Insert("Shared", 8U);
    auto& override = root.m_Nodes[1].m_Overrides.ExpandAndGetRef();
    override.m_Path.PushBack(nested.m_Nodes[0].m_Id);
    override.m_Path.PushBack(leaf.m_Nodes[0].m_Id);
    override.m_Properties.Insert("Value", 42U);
    const auto resolver = [&](const xiiUuid& id) -> const xiiGraphDefinition* {
      if (id == leaf.m_Id)
        return &leaf;
      if (id == nested.m_Id)
        return &nested;
      if (id == root.m_Id)
        return &root;
      return nullptr;
    };
    xiiGraphExpansion expanded;
    XII_TEST_RESULT(xiiGraphComposition::Expand(root, resolver, expanded));
    XII_TEST_INT(expanded.m_Graph.GetVertexCount(), 4);
    XII_TEST_INT(expanded.m_Graph.GetEdgeCount(), 4);
    XII_TEST_INT(expanded.m_Dependencies.GetCount(), 3);
    XII_TEST_INT(expanded.m_Nodes[0].m_Properties["Value"].Get<xiiUInt32>(), 1);
    XII_TEST_INT(expanded.m_Nodes[2].m_Properties["Value"].Get<xiiUInt32>(), 42);
    XII_TEST_INT(expanded.m_Nodes[2].m_Parameters["Shared"].Get<xiiUInt32>(), 8);
    XII_TEST_BOOL(expanded.m_Nodes[0].m_Source.m_Element == expanded.m_Nodes[2].m_Source.m_Element);
    XII_TEST_BOOL(expanded.m_Nodes[0].m_Source.m_InstancePath != expanded.m_Nodes[2].m_Source.m_InstancePath);
    XII_TEST_INT(expanded.m_Nodes[0].m_Source.m_InstancePath.GetCount(), 2);
    XII_TEST_BOOL(xiiGraphComposition::IsCurrent(expanded.m_Dependencies, resolver));
    ++leaf.m_uiPresentationRevision;
    XII_TEST_BOOL(xiiGraphComposition::IsCurrent(expanded.m_Dependencies, resolver));
    ++leaf.m_uiParameterRevision;
    XII_TEST_BOOL(!xiiGraphComposition::IsCurrent(expanded.m_Dependencies, resolver));
    XII_TEST_BOOL(xiiGraphComposition::IsCurrent(expanded.m_Dependencies, resolver, xiiGraphDefinitionDependency::Resources | xiiGraphDefinitionDependency::Topology | xiiGraphDefinitionDependency::Definition));
    ++leaf.m_uiResourceRevision;
    XII_TEST_BOOL(!xiiGraphComposition::IsCurrent(expanded.m_Dependencies, resolver, xiiGraphDefinitionDependency::Resources));
    const auto        old = expanded.m_Nodes[0].m_hVertex;
    xiiGraphHandleMap mapping;
    XII_TEST_RESULT(expanded.m_Graph.Compact(mapping));
    XII_TEST_RESULT(expanded.Remap(mapping));
    XII_TEST_BOOL(expanded.m_Graph.IsValid(expanded.m_Nodes[0].m_hVertex));
    XII_TEST_BOOL(expanded.m_Nodes[0].m_hVertex != old);
    for (const auto& boundary : expanded.m_Boundary)
      XII_TEST_BOOL(expanded.m_Graph.IsValid(boundary.m_hVertex));
    xiiGraphExpansion again;
    XII_TEST_RESULT(xiiGraphComposition::Expand(root, resolver, again));
    for (xiiUInt32 i = 0; i < again.m_Nodes.GetCount(); ++i)
    {
      XII_TEST_BOOL(again.m_Nodes[i].m_Source.m_InstancePath == expanded.m_Nodes[i].m_Source.m_InstancePath);
      XII_TEST_BOOL(again.m_Nodes[i].m_Properties == expanded.m_Nodes[i].m_Properties);
    }
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Definition recursion is distinct from topology cycles; failures preserve output")
  {
    auto definition = GraphCompositionTest::Make(4, 1);
    GraphCompositionTest::Link(definition, 0, 0);
    xiiGraphExpansion expanded;
    XII_TEST_RESULT(xiiGraphComposition::Expand(definition, {}, expanded));
    xiiDynamicArray<xiiGraphVertexHandle> order;
    XII_TEST_BOOL(xiiGraphAlgorithms::TopologicalSort(expanded.m_Graph, order).Failed());
    const auto preserved               = expanded.m_Nodes[0].m_hVertex;
    definition.m_Nodes[0].m_Definition = definition.m_Id;
    xiiStringBuilder error;
    const auto       resolver = [&](const xiiUuid&) {
      return &definition;
    };
    XII_TEST_BOOL(xiiGraphComposition::Expand(definition, resolver, expanded, &error).Failed());
    XII_TEST_BOOL(error.FindSubString("Recursive") != nullptr);
    XII_TEST_BOOL(error.FindSubString("instance path") != nullptr);
    XII_TEST_BOOL(expanded.m_Graph.IsValid(preserved));
    XII_TEST_BOOL(xiiGraphComposition::Expand(definition, {}, expanded, &error).Failed());
    XII_TEST_BOOL(error.FindSubString("Missing") != nullptr);
    definition.m_Nodes[0].m_Definition = {};
    xiiGraphComposition::Limits limits;
    limits.m_uiNodes = 0;
    XII_TEST_BOOL(xiiGraphComposition::Expand(definition, {}, limits, expanded, &error).Failed());
    XII_TEST_BOOL(expanded.m_Graph.IsValid(preserved));
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Reflected definition serialization preserves document identities")
  {
    const auto                    definition = GraphCompositionTest::Make(5, 2);
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    xiiReflectionSerializer::WriteObjectToBinary(writer, xiiGetStaticRTTI<xiiGraphDefinition>(), &definition);
    xiiMemoryStreamReader reader(&storage);
    xiiGraphDefinition    loaded;
    xiiReflectionSerializer::ReadObjectPropertiesFromBinary(reader, *xiiGetStaticRTTI<xiiGraphDefinition>(), &loaded);
    XII_TEST_RESULT(loaded.Validate());
    XII_TEST_BOOL(loaded.m_Id == definition.m_Id);
    XII_TEST_BOOL(loaded.m_Nodes[1].m_Id == definition.m_Nodes[1].m_Id);
  }
}
