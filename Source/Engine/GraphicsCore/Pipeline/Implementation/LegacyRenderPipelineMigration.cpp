/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Implementation/LegacyRenderPipelineMigration.h>

namespace
{
  constexpr xiiStringView s_Resource = "RenderPipelines/ScreenEffects.xiiBinRenderPipeline"_xiisv;
  bool                    IsLegacyStage(xiiStringView type, const xiiVariantDictionary& properties, xiiStringView stage)
  {
    const auto* selected = properties.GetValue("Stage");
    return type == "xiiBuiltinRenderPipelinePass" && selected && selected->IsA<xiiString>() && selected->Get<xiiString>() == stage;
  }
  xiiUuid ConnectionId(xiiUuid source, xiiUuid target, xiiStringView port)
  {
    source.HashCombine(target);
    source.HashCombine(xiiUuid::MakeStableUuidFromString(xiiStringBuilder("ScreenEffects/FrameInput/", port)));
    return source;
  }
  template <typename Nodes, typename AddEdge>
  bool Convert(Nodes& nodes, AddEdge add)
  {
    xiiUInt32 visibility = xiiInvalidIndex, lighting = xiiInvalidIndex;
    for (xiiUInt32 index = 0; index < nodes.GetCount(); ++index)
    {
      if (IsLegacyStage(nodes[index].m_sType, nodes[index].m_Properties, "Visibility"))
        visibility = index;
      if (IsLegacyStage(nodes[index].m_sType, nodes[index].m_Properties, "Lighting"))
        lighting = index;
    }
    bool changed = false;
    for (xiiUInt32 index = 0; index < nodes.GetCount(); ++index)
    {
      auto& node = nodes[index];
      if (!IsLegacyStage(node.m_sType, node.m_Properties, "ScreenEffects"))
        continue;
      xiiVariant pins;
      const bool hasPins = node.m_Properties.TryGetValue("PinConfigurations", pins);
      node.m_Properties.Clear();
      if (hasPins)
        node.m_Properties.Insert("PinConfigurations", pins);
      node.m_sType.Clear();
      node.m_Definition = xiiScreenEffectsDefinitionId();
      changed           = true;
      if (visibility != xiiInvalidIndex)
      {
        add(visibility, index, "PerFrameCamera");
        add(visibility, index, "PerFrameGlobal");
        add(visibility, index, "PerFrameLight");
      }
      if (lighting != xiiInvalidIndex)
        add(lighting, index, "HardwareGI");
    }
    return changed;
  }

  constexpr xiiStringView s_DebugResource = "RenderPipelines/DebugVisualization.xiiBinRenderPipeline"_xiisv;

  template <typename Nodes, typename AddEdge>
  bool ConvertDebug(Nodes& nodes, AddEdge add)
  {
    xiiUInt32 preparation = xiiInvalidIndex, visibility = xiiInvalidIndex;
    for (xiiUInt32 index = 0; index < nodes.GetCount(); ++index)
    {
      if (IsLegacyStage(nodes[index].m_sType, nodes[index].m_Properties, "PrepareFrame"))
        preparation = index;
      if (IsLegacyStage(nodes[index].m_sType, nodes[index].m_Properties, "Visibility"))
        visibility = index;
    }
    bool changed = false;
    for (xiiUInt32 index = 0; index < nodes.GetCount(); ++index)
    {
      auto& node = nodes[index];
      if (!IsLegacyStage(node.m_sType, node.m_Properties, "Debug"))
        continue;
      xiiVariant pins;
      const bool hasPins = node.m_Properties.TryGetValue("PinConfigurations", pins);
      node.m_Properties.Clear();
      if (hasPins)
        node.m_Properties.Insert("PinConfigurations", pins);
      node.m_sType.Clear();
      node.m_Definition = xiiDebugVisualizationDefinitionId();
      changed           = true;
      if (preparation != xiiInvalidIndex)
        for (xiiStringView port : {"SceneInstances", "SkinningMatrices", "VisibleMeshlets", "VisibleMeshletCount", "MeshIndirectCommands", "MeshIndirectCommandCount", "GeometryMetadata", "MeshletMetadata", "SurfaceMaterials"})
          add(preparation, index, port);
      if (visibility != xiiInvalidIndex)
        add(visibility, index, "PerFrameCamera");
    }
    return changed;
  }

  class DebugVisualizationSourcePatch : public xiiGraphPatch
  {
  public:
    DebugVisualizationSourcePatch() : xiiGraphPatch("xiiBuiltinRenderPipelinePass", 5) {}
    void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph*, xiiAbstractObjectNode* node) const override
    {
      const auto* stage = node->FindProperty("Stage");
      if (!stage || stage->m_Value != xiiVariant("Debug"))
        return;
      if (const auto* parameters = node->FindProperty("ParameterOverrides"))
        node->AddProperty("Parameters", parameters->m_Value);
      node->RemoveProperty("Stage");
      node->AddProperty("Resource", xiiString(s_DebugResource));
      context.RenameClass("xiiRenderPipelineSubgraphPass", 1);
    }
  };
  static DebugVisualizationSourcePatch s_DebugSourcePatch;

  class DebugVisualizationConnectionsPatch : public xiiGraphPatch
  {
  public:
    DebugVisualizationConnectionsPatch() : xiiGraphPatch({}, 3, PatchType::GraphPatch) {}
    void Patch(xiiGraphPatchContext&, xiiAbstractObjectGraph* graph, xiiAbstractObjectNode*) const override
    {
      auto* root = graph->GetNodeByName("ObjectTree");
      if (!root)
        return;
      const auto* childrenProperty = root->FindProperty("Children");
      if (!childrenProperty || !childrenProperty->m_Value.IsA<xiiVariantArray>())
        return;
      auto                     children = childrenProperty->m_Value.Get<xiiVariantArray>();
      xiiDynamicArray<xiiUuid> debugNodes;
      xiiUuid                  preparation, visibility;
      for (auto entry : graph->GetAllNodes())
      {
        const auto* node  = entry.Value();
        const auto* stage = node->FindProperty("Stage");
        if (node->GetType() != "xiiBuiltinRenderPipelinePass" || !stage)
          continue;
        if (stage->m_Value == xiiVariant("PrepareFrame"))
          preparation = node->GetGuid();
        if (stage->m_Value == xiiVariant("Visibility"))
          visibility = node->GetGuid();
        if (stage->m_Value == xiiVariant("Debug"))
          debugNodes.PushBack(node->GetGuid());
      }
      const auto add = [&](xiiUuid source, xiiUuid target, xiiStringView port) {
        if (!source.IsValid())
          return;
        for (auto entry : graph->GetAllNodes())
        {
          const auto* edge        = entry.Value();
          const auto* destination = edge->FindProperty("Target");
          const auto* input       = edge->FindProperty("TargetPin");
          if (destination && input && destination->m_Value == xiiVariant(target) && input->m_Value == xiiVariant(port))
            return;
        }
        const auto id = ConnectionId(source, target, port);
        if (graph->GetNode(id))
          return;
        auto* edge = graph->AddNode(id, "xiiDocumentObject_ConnectionBase", 1);
        edge->AddProperty("Source", source);
        edge->AddProperty("Target", target);
        edge->AddProperty("SourcePin", xiiString(port));
        edge->AddProperty("TargetPin", xiiString(port));
        children.PushBack(id);
      };
      for (auto debug : debugNodes)
      {
        for (xiiStringView port : {"SceneInstances", "SkinningMatrices", "VisibleMeshlets", "VisibleMeshletCount", "MeshIndirectCommands", "MeshIndirectCommandCount", "GeometryMetadata", "MeshletMetadata", "SurfaceMaterials"})
          add(preparation, debug, port);
        add(visibility, debug, "PerFrameCamera");
      }
      root->ChangeProperty("Children", children);
    }
  };
  static DebugVisualizationConnectionsPatch s_DebugConnectionsPatch;

  class ScreenEffectsSourcePatch : public xiiGraphPatch
  {
  public:
    ScreenEffectsSourcePatch() : xiiGraphPatch("xiiBuiltinRenderPipelinePass", 4) {}
    void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph*, xiiAbstractObjectNode* node) const override
    {
      const auto* stage = node->FindProperty("Stage");
      if (!stage || stage->m_Value != xiiVariant("ScreenEffects"))
        return;
      if (const auto* parameters = node->FindProperty("ParameterOverrides"))
        node->AddProperty("Parameters", parameters->m_Value);
      node->RemoveProperty("Stage");
      node->AddProperty("Resource", xiiString(s_Resource));
      context.RenameClass("xiiRenderPipelineSubgraphPass", 1);
    }
  };
  static ScreenEffectsSourcePatch s_SourcePatch;

  /// Materialize formerly implicit uniform and availability dependencies in source documents.
  class ScreenEffectsSourceConnectionsPatch : public xiiGraphPatch
  {
  public:
    ScreenEffectsSourceConnectionsPatch() : xiiGraphPatch({}, 2, PatchType::GraphPatch) {}
    void Patch(xiiGraphPatchContext&, xiiAbstractObjectGraph* graph, xiiAbstractObjectNode*) const override
    {
      auto* root = graph->GetNodeByName("ObjectTree");
      if (!root)
        return;
      const auto* childrenProperty = root->FindProperty("Children");
      if (!childrenProperty || !childrenProperty->m_Value.IsA<xiiVariantArray>())
        return;
      auto                     children = childrenProperty->m_Value.Get<xiiVariantArray>();
      xiiDynamicArray<xiiUuid> screens;
      xiiUuid                  visibility, lighting;
      for (auto entry : graph->GetAllNodes())
      {
        const auto* node  = entry.Value();
        const auto* stage = node->FindProperty("Stage");
        if (node->GetType() != "xiiBuiltinRenderPipelinePass" || !stage)
          continue;
        if (stage->m_Value == xiiVariant("Visibility"))
          visibility = node->GetGuid();
        if (stage->m_Value == xiiVariant("Lighting"))
          lighting = node->GetGuid();
        if (stage->m_Value == xiiVariant("ScreenEffects"))
          screens.PushBack(node->GetGuid());
      }
      const auto add = [&](xiiUuid source, xiiUuid target, xiiStringView port) {
        if (!source.IsValid())
          return;
        for (auto entry : graph->GetAllNodes())
        {
          const auto* edge        = entry.Value();
          const auto* destination = edge->FindProperty("Target");
          const auto* input       = edge->FindProperty("TargetPin");
          if (destination && input && destination->m_Value == xiiVariant(target) && input->m_Value == xiiVariant(port))
            return;
        }
        const auto id = ConnectionId(source, target, port);
        if (graph->GetNode(id))
          return;
        auto* edge = graph->AddNode(id, "xiiDocumentObject_ConnectionBase", 1);
        edge->AddProperty("Source", source);
        edge->AddProperty("Target", target);
        edge->AddProperty("SourcePin", xiiString(port));
        edge->AddProperty("TargetPin", xiiString(port));
        children.PushBack(id);
      };
      for (auto screen : screens)
      {
        add(visibility, screen, "PerFrameCamera");
        add(visibility, screen, "PerFrameGlobal");
        add(visibility, screen, "PerFrameLight");
        add(lighting, screen, "HardwareGI");
      }
      root->ChangeProperty("Children", children);
    }
  };
  static ScreenEffectsSourceConnectionsPatch s_ConnectionsPatch;
} // namespace

void xiiMigrateLegacyRenderPipelineDefinitions(xiiRenderPipelineResourceDescriptor& descriptor)
{
  const auto addRoot = [&](xiiUInt32 source, xiiUInt32 target, xiiStringView port) {
    for (const auto& edge : descriptor.m_Connections)
      if (edge.m_uiTarget == target && edge.m_sInput == port)
        return;
    if (descriptor.Connect(source, port, target, port).Succeeded())
      descriptor.m_Connections.PeekBack().m_Id = ConnectionId(descriptor.m_Passes[source].m_Id, descriptor.m_Passes[target].m_Id, port);
  };
  bool debugChanged = ConvertDebug(descriptor.m_Passes, addRoot);
  bool changed      = Convert(descriptor.m_Passes, addRoot);
  for (auto& definition : descriptor.m_Definitions)
  {
    const auto add = [&](xiiUInt32 source, xiiUInt32 target, xiiStringView port) {
      for (xiiUInt32 index = 0; index < definition.m_Edges.GetCount(); ++index)
        if (definition.m_Topology.m_EdgeTargets[index] == target && definition.m_Edges[index].m_sTargetPort == port)
          return;
      xiiGraphDefinitionEdge edge;
      edge.m_Id          = ConnectionId(definition.m_Nodes[source].m_Id, definition.m_Nodes[target].m_Id, port);
      edge.m_sSourcePort = port;
      edge.m_sTargetPort = port;
      definition.m_Topology.m_EdgeSources.PushBack(source);
      definition.m_Topology.m_EdgeTargets.PushBack(target);
      definition.m_Edges.PushBack(std::move(edge));
    };
    debugChanged |= ConvertDebug(definition.m_Nodes, add);
    changed |= Convert(definition.m_Nodes, add);
  }
  if (debugChanged)
  {
    bool found = false;
    for (const auto& reference : descriptor.m_References)
      found |= reference.m_Id == xiiDebugVisualizationDefinitionId();
    if (!found)
      descriptor.m_References.PushBack({xiiDebugVisualizationDefinitionId(), s_DebugResource});
  }
  if (changed)
  {
    for (const auto& reference : descriptor.m_References)
      if (reference.m_Id == xiiScreenEffectsDefinitionId())
        return;
    descriptor.m_References.PushBack({xiiScreenEffectsDefinitionId(), s_Resource});
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_LegacyRenderPipelineMigration);
