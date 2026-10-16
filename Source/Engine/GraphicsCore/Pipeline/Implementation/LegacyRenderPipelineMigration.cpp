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

  constexpr xiiStringView s_OpaqueResource = "RenderPipelines/OpaqueComposition.xiiBinRenderPipeline"_xiisv;

  template <typename Nodes, typename AddEdge>
  bool ConvertOpaque(Nodes& nodes, AddEdge add)
  {
    xiiUInt32 lighting = xiiInvalidIndex, visibility = xiiInvalidIndex;
    for (xiiUInt32 index = 0; index < nodes.GetCount(); ++index)
    {
      if (IsLegacyStage(nodes[index].m_sType, nodes[index].m_Properties, "Lighting"))
        lighting = index;
      if (IsLegacyStage(nodes[index].m_sType, nodes[index].m_Properties, "Visibility"))
        visibility = index;
    }
    bool changed = false;
    for (xiiUInt32 index = 0; index < nodes.GetCount(); ++index)
    {
      auto& node = nodes[index];
      if (!IsLegacyStage(node.m_sType, node.m_Properties, "OpaqueComposition"))
        continue;
      xiiVariant pins;
      const bool hasPins = node.m_Properties.TryGetValue("PinConfigurations", pins);
      node.m_Properties.Clear();
      if (hasPins)
        node.m_Properties.Insert("PinConfigurations", pins);
      node.m_sType.Clear();
      node.m_Definition = xiiOpaqueCompositionDefinitionId();
      changed           = true;
      if (lighting != xiiInvalidIndex)
        for (xiiStringView port : {"RTFinalGI", "RTFinalReflections", "VolumetricScattering"})
          add(lighting, index, port);
      if (visibility != xiiInvalidIndex)
      {
        add(visibility, index, "PerFrameCamera");
        add(visibility, index, "PerFrameGlobal");
      }
    }
    return changed;
  }

  constexpr xiiStringView s_TransparencyResource = "RenderPipelines/TransparencyResolve.xiiBinRenderPipeline"_xiisv;

  template <typename Nodes>
  bool ConvertTransparency(Nodes& nodes)
  {
    bool changed = false;
    for (auto& node : nodes)
    {
      if (!IsLegacyStage(node.m_sType, node.m_Properties, "Transparency"))
        continue;
      xiiVariant pins;
      const bool hasPins = node.m_Properties.TryGetValue("PinConfigurations", pins);
      node.m_Properties.Clear();
      if (hasPins)
        node.m_Properties.Insert("PinConfigurations", pins);
      node.m_sType.Clear();
      node.m_Definition = xiiTransparencyResolveDefinitionId();
      changed           = true;
    }
    return changed;
  }

  constexpr xiiStringView s_AmbientOcclusionResource = "RenderPipelines/AmbientOcclusion.xiiBinRenderPipeline"_xiisv;

  template <typename Nodes, typename VisitEdges, typename AddEdge, typename Boundaries>
  bool ConvertAmbientOcclusion(Nodes& nodes, VisitEdges visit, AddEdge add, Boundaries& boundaries)
  {
    bool            changed       = false;
    const xiiUInt32 originalCount = nodes.GetCount();
    const auto      findStage     = [&](xiiStringView stage) {
      for (xiiUInt32 i = 0; i < originalCount; ++i)
        if (IsLegacyStage(nodes[i].m_sType, nodes[i].m_Properties, stage))
          return i;
      return xiiInvalidIndex;
    };
    for (xiiUInt32 preparation = 0; preparation < originalCount; ++preparation)
    {
      if (!IsLegacyStage(nodes[preparation].m_sType, nodes[preparation].m_Properties, "LightingPreparation"))
        continue;
      auto id = nodes[preparation].m_Id;
      id.HashCombine(xiiAmbientOcclusionDefinitionId());
      bool exists = false;
      for (const auto& node : nodes)
        exists |= node.m_Id == id;
      if (exists)
        continue;
      xiiUInt32 lighting = findStage("Lighting"), geometry = findStage("DepthAndGeometry");
      visit([&](auto& source, auto& target, auto& output, auto& input) {
        if (source >= originalCount || target >= originalCount)
          return;
        if (source == preparation && IsLegacyStage(nodes[target].m_sType, nodes[target].m_Properties, "Lighting"))
          lighting = target;
        if (target == preparation && input == "SceneDepth")
          geometry = source;
      });
      const xiiUInt32 ao   = nodes.GetCount();
      auto&           node = nodes.ExpandAndGetRef();
      node.m_Id            = id;
      node.m_sName         = "AmbientOcclusion";
      node.m_Definition    = xiiAmbientOcclusionDefinitionId();
      visit([&](auto& source, auto&, auto& output, auto&) {
        if (source == preparation && output == "RawAO")
          source = ao;
      });
      for (auto& mapping : boundaries)
        if (!mapping.m_bInput && mapping.m_Node == nodes[preparation].m_Id && mapping.m_sPort == "RawAO")
          mapping.m_Node = id;
      // Keep authored output contracts on the new subgraph interface.
      if (const auto* value = nodes[preparation].m_Properties.GetValue("PinConfigurations"); value && value->IsA<xiiVariantArray>())
      {
        xiiVariantArray retained, moved;
        for (const auto& configuration : value->Get<xiiVariantArray>())
        {
          const auto* metadata = configuration.IsA<xiiVariantDictionary>() ? configuration.Get<xiiVariantDictionary>().GetValue("Metadata") : nullptr;
          const auto* name     = metadata && metadata->IsA<xiiVariantDictionary>() ? metadata->Get<xiiVariantDictionary>().GetValue("Name") : nullptr;
          const auto* input    = metadata && metadata->IsA<xiiVariantDictionary>() ? metadata->Get<xiiVariantDictionary>().GetValue("Input") : nullptr;
          if (name && input && *name == xiiVariant("RawAO") && *input == xiiVariant(false))
            moved.PushBack(configuration);
          else
            retained.PushBack(configuration);
        }
        nodes[preparation].m_Properties.Insert("PinConfigurations", retained);
        if (!moved.IsEmpty())
          nodes[ao].m_Properties.Insert("PinConfigurations", moved);
      }
      if (geometry != xiiInvalidIndex)
        for (xiiStringView port : {"SceneDepth", "GBufferNormal", "NormalRoughness", "DilatedVelocityBuffer"})
          add(geometry, port, ao, port);
      const auto visibility = findStage("Visibility"), frame = findStage("PrepareFrame");
      if (visibility != xiiInvalidIndex)
        for (xiiStringView port : {"PerFrameCamera", "PerFrameGlobal"})
          add(visibility, port, ao, port);
      if (frame != xiiInvalidIndex)
        for (xiiStringView port : {"RayTracingScene", "RayTracingMaterialData", "RayTracingGeometryData"})
          add(frame, port, ao, port);
      add(preparation, "After", ao, "Before");
      if (lighting != xiiInvalidIndex)
      {
        add(ao, "StableAO", lighting, "StableAO");
        add(ao, "After", lighting, "Before");
      }
      changed = true;
    }
    return changed;
  }

  bool ConvertAmbientOcclusion(xiiRenderPipelineResourceDescriptor& descriptor)
  {
    const auto visit = [&](auto callback) {
      for (auto& edge : descriptor.m_Connections)
        callback(edge.m_uiSource, edge.m_uiTarget, edge.m_sOutput, edge.m_sInput);
    };
    const auto add = [&](xiiUInt32 source, xiiStringView output, xiiUInt32 target, xiiStringView input) {
      for (const auto& edge : descriptor.m_Connections)
        if (edge.m_uiSource == source && edge.m_uiTarget == target && edge.m_sOutput == output && edge.m_sInput == input)
          return;
      if (descriptor.Connect(source, output, target, input).Succeeded())
        descriptor.m_Connections.PeekBack().m_Id = ConnectionId(descriptor.m_Passes[source].m_Id, descriptor.m_Passes[target].m_Id, xiiStringBuilder("AO/", output, "/", input));
    };
    return ConvertAmbientOcclusion(descriptor.m_Passes, visit, add, descriptor.m_Interface);
  }

  /// Extract the formerly implicit AO operations without changing existing node or edge identities.
  class AmbientOcclusionSourcePatch : public xiiGraphPatch
  {
  public:
    AmbientOcclusionSourcePatch() : xiiGraphPatch({}, 5, PatchType::GraphPatch) {}
    void Patch(xiiGraphPatchContext&, xiiAbstractObjectGraph* graph, xiiAbstractObjectNode*) const override
    {
      auto*       root     = graph->GetNodeByName("ObjectTree");
      const auto* property = root ? root->FindProperty("Children") : nullptr;
      if (!property || !property->m_Value.IsA<xiiVariantArray>())
        return;
      auto                                children = property->m_Value.Get<xiiVariantArray>();
      xiiRenderPipelineResourceDescriptor descriptor;
      xiiMap<xiiUuid, xiiUInt32>          indices;
      for (const auto& child : children)
      {
        auto* node = child.IsA<xiiUuid>() ? graph->GetNode(child.Get<xiiUuid>()) : nullptr;
        if (!node || node->FindProperty("Source"))
          continue;
        auto& pass   = descriptor.m_Passes.ExpandAndGetRef();
        pass.m_Id    = node->GetGuid();
        pass.m_sType = node->GetType();
        if (const auto* stage = node->FindProperty("Stage"))
          pass.m_Properties.Insert("Stage", stage->m_Value);
        indices.Insert(pass.m_Id, descriptor.m_Passes.GetCount() - 1);
      }
      for (const auto& child : children)
      {
        auto*       node   = child.IsA<xiiUuid>() ? graph->GetNode(child.Get<xiiUuid>()) : nullptr;
        const auto* source = node ? node->FindProperty("Source") : nullptr;
        const auto* target = node ? node->FindProperty("Target") : nullptr;
        const auto* output = node ? node->FindProperty("SourcePin") : nullptr;
        const auto* input  = node ? node->FindProperty("TargetPin") : nullptr;
        xiiUInt32   sourceIndex, targetIndex;
        if (!source || !target || !output || !input || !source->m_Value.IsA<xiiUuid>() || !target->m_Value.IsA<xiiUuid>() ||
            !output->m_Value.IsA<xiiString>() || !input->m_Value.IsA<xiiString>() ||
            !indices.TryGetValue(source->m_Value.Get<xiiUuid>(), sourceIndex) || !indices.TryGetValue(target->m_Value.Get<xiiUuid>(), targetIndex))
          continue;
        descriptor.m_Connections.PushBack({sourceIndex, targetIndex, output->m_Value.Get<xiiString>(), input->m_Value.Get<xiiString>(), node->GetGuid()});
      }
      const auto originalCount = descriptor.m_Passes.GetCount();
      if (!ConvertAmbientOcclusion(descriptor))
        return;
      for (xiiUInt32 i = originalCount; i < descriptor.m_Passes.GetCount(); ++i)
      {
        const auto& pass = descriptor.m_Passes[i];
        auto*       node = graph->AddNode(pass.m_Id, "xiiRenderPipelineSubgraphPass", 1);
        node->AddProperty("Resource", xiiString(s_AmbientOcclusionResource));
        for (xiiUInt32 source = 0; source < originalCount; ++source)
        {
          auto expected = descriptor.m_Passes[source].m_Id;
          expected.HashCombine(xiiAmbientOcclusionDefinitionId());
          if (expected != pass.m_Id)
            continue;
          auto*       original = graph->GetNode(descriptor.m_Passes[source].m_Id);
          const auto* pins     = original ? original->FindProperty("PinConfigurations") : nullptr;
          if (!pins || !pins->m_Value.IsA<xiiVariantArray>())
            continue;
          xiiVariantArray retained, moved;
          for (const auto& configuration : pins->m_Value.Get<xiiVariantArray>())
          {
            const auto* config     = configuration.IsA<xiiUuid>() ? graph->GetNode(configuration.Get<xiiUuid>()) : nullptr;
            const auto* metadataId = config ? config->FindProperty("Metadata") : nullptr;
            const auto* metadata   = metadataId && metadataId->m_Value.IsA<xiiUuid>() ? graph->GetNode(metadataId->m_Value.Get<xiiUuid>()) : nullptr;
            const auto* name       = metadata ? metadata->FindProperty("Name") : nullptr;
            const auto* input      = metadata ? metadata->FindProperty("Input") : nullptr;
            if (name && input && name->m_Value == xiiVariant("RawAO") && input->m_Value == xiiVariant(false))
              moved.PushBack(configuration);
            else
              retained.PushBack(configuration);
          }
          original->ChangeProperty("PinConfigurations", retained);
          if (!moved.IsEmpty())
            node->AddProperty("PinConfigurations", moved);
        }
        children.PushBack(pass.m_Id);
      }
      for (const auto& edge : descriptor.m_Connections)
      {
        auto* node = graph->GetNode(edge.m_Id);
        if (!node)
        {
          node = graph->AddNode(edge.m_Id, "xiiDocumentObject_ConnectionBase", 1);
          children.PushBack(edge.m_Id);
        }
        const auto set = [&](const char* name, const xiiVariant& value) {
          if (node->FindProperty(name))
            node->ChangeProperty(name, value);
          else
            node->AddProperty(name, value);
        };
        set("Source", descriptor.m_Passes[edge.m_uiSource].m_Id);
        set("Target", descriptor.m_Passes[edge.m_uiTarget].m_Id);
        set("SourcePin", edge.m_sOutput);
        set("TargetPin", edge.m_sInput);
      }
      root->ChangeProperty("Children", children);
    }
  };
  static AmbientOcclusionSourcePatch s_AmbientOcclusionSourcePatch;

  class TransparencyResolveSourcePatch : public xiiGraphPatch
  {
  public:
    TransparencyResolveSourcePatch() : xiiGraphPatch("xiiBuiltinRenderPipelinePass", 7) {}
    void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph*, xiiAbstractObjectNode* node) const override
    {
      const auto* stage = node->FindProperty("Stage");
      if (!stage || stage->m_Value != xiiVariant("Transparency"))
        return;
      if (const auto* parameters = node->FindProperty("ParameterOverrides"))
        node->AddProperty("Parameters", parameters->m_Value);
      node->RemoveProperty("Stage");
      node->AddProperty("Resource", xiiString(s_TransparencyResource));
      context.RenameClass("xiiRenderPipelineSubgraphPass", 1);
    }
  };
  static TransparencyResolveSourcePatch s_TransparencySourcePatch;

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

  class OpaqueCompositionSourcePatch : public xiiGraphPatch
  {
  public:
    OpaqueCompositionSourcePatch() : xiiGraphPatch("xiiBuiltinRenderPipelinePass", 6) {}
    void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph*, xiiAbstractObjectNode* node) const override
    {
      const auto* stage = node->FindProperty("Stage");
      if (!stage || stage->m_Value != xiiVariant("OpaqueComposition"))
        return;
      if (const auto* parameters = node->FindProperty("ParameterOverrides"))
        node->AddProperty("Parameters", parameters->m_Value);
      node->RemoveProperty("Stage");
      node->AddProperty("Resource", xiiString(s_OpaqueResource));
      context.RenameClass("xiiRenderPipelineSubgraphPass", 1);
    }
  };
  static OpaqueCompositionSourcePatch s_OpaqueSourcePatch;

  class OpaqueCompositionConnectionsPatch : public xiiGraphPatch
  {
  public:
    OpaqueCompositionConnectionsPatch() : xiiGraphPatch({}, 4, PatchType::GraphPatch) {}
    void Patch(xiiGraphPatchContext&, xiiAbstractObjectGraph* graph, xiiAbstractObjectNode*) const override
    {
      auto* root = graph->GetNodeByName("ObjectTree");
      if (!root)
        return;
      const auto* childrenProperty = root->FindProperty("Children");
      if (!childrenProperty || !childrenProperty->m_Value.IsA<xiiVariantArray>())
        return;
      auto                     children = childrenProperty->m_Value.Get<xiiVariantArray>();
      xiiDynamicArray<xiiUuid> opaqueNodes;
      xiiUuid                  lighting, visibility;
      for (auto entry : graph->GetAllNodes())
      {
        const auto* node  = entry.Value();
        const auto* stage = node->FindProperty("Stage");
        if (node->GetType() != "xiiBuiltinRenderPipelinePass" || !stage)
          continue;
        if (stage->m_Value == xiiVariant("Lighting"))
          lighting = node->GetGuid();
        if (stage->m_Value == xiiVariant("Visibility"))
          visibility = node->GetGuid();
        if (stage->m_Value == xiiVariant("OpaqueComposition"))
          opaqueNodes.PushBack(node->GetGuid());
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
      for (auto debug : opaqueNodes)
      {
        for (xiiStringView port : {"RTFinalGI", "RTFinalReflections", "VolumetricScattering"})
          add(lighting, debug, port);
        add(visibility, debug, "PerFrameCamera");
        add(visibility, debug, "PerFrameGlobal");
      }
      root->ChangeProperty("Children", children);
    }
  };
  static OpaqueCompositionConnectionsPatch s_OpaqueConnectionsPatch;

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

void xiiMigrateLegacyRenderPipelineDefinitions(xiiRenderPipelineResourceDescriptor& descriptor, bool extractAmbientOcclusion)
{
  const auto addRoot = [&](xiiUInt32 source, xiiUInt32 target, xiiStringView port) {
    for (const auto& edge : descriptor.m_Connections)
      if (edge.m_uiTarget == target && edge.m_sInput == port)
        return;
    if (descriptor.Connect(source, port, target, port).Succeeded())
      descriptor.m_Connections.PeekBack().m_Id = ConnectionId(descriptor.m_Passes[source].m_Id, descriptor.m_Passes[target].m_Id, port);
  };
  bool ambientOcclusionChanged = extractAmbientOcclusion && ConvertAmbientOcclusion(descriptor);
  bool transparencyChanged     = ConvertTransparency(descriptor.m_Passes);
  bool opaqueChanged           = ConvertOpaque(descriptor.m_Passes, addRoot);
  bool debugChanged            = ConvertDebug(descriptor.m_Passes, addRoot);
  bool changed                 = Convert(descriptor.m_Passes, addRoot);
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
    const auto visitAO = [&](auto callback) {
      for (xiiUInt32 i = 0; i < definition.m_Edges.GetCount(); ++i)
        callback(definition.m_Topology.m_EdgeSources[i], definition.m_Topology.m_EdgeTargets[i], definition.m_Edges[i].m_sSourcePort, definition.m_Edges[i].m_sTargetPort);
    };
    const auto addAO = [&](xiiUInt32 source, xiiStringView output, xiiUInt32 target, xiiStringView input) {
      for (xiiUInt32 i = 0; i < definition.m_Edges.GetCount(); ++i)
        if (definition.m_Topology.m_EdgeSources[i] == source && definition.m_Topology.m_EdgeTargets[i] == target && definition.m_Edges[i].m_sSourcePort == output && definition.m_Edges[i].m_sTargetPort == input)
          return;
      xiiGraphDefinitionEdge edge;
      edge.m_Id          = ConnectionId(definition.m_Nodes[source].m_Id, definition.m_Nodes[target].m_Id, xiiStringBuilder("AO/", output, "/", input));
      edge.m_sSourcePort = output;
      edge.m_sTargetPort = input;
      definition.m_Topology.m_EdgeSources.PushBack(source);
      definition.m_Topology.m_EdgeTargets.PushBack(target);
      definition.m_Edges.PushBack(std::move(edge));
    };
    if (extractAmbientOcclusion && ConvertAmbientOcclusion(definition.m_Nodes, visitAO, addAO, definition.m_Boundary))
    {
      definition.m_Topology.m_uiVertexCount = definition.m_Nodes.GetCount();
      ambientOcclusionChanged               = true;
    }
    transparencyChanged |= ConvertTransparency(definition.m_Nodes);
    opaqueChanged |= ConvertOpaque(definition.m_Nodes, add);
    debugChanged |= ConvertDebug(definition.m_Nodes, add);
    changed |= Convert(definition.m_Nodes, add);
  }
  if (ambientOcclusionChanged)
  {
    bool found = false;
    for (const auto& reference : descriptor.m_References)
      found |= reference.m_Id == xiiAmbientOcclusionDefinitionId();
    if (!found)
      descriptor.m_References.PushBack({xiiAmbientOcclusionDefinitionId(), s_AmbientOcclusionResource});
  }
  if (transparencyChanged)
  {
    bool found = false;
    for (const auto& reference : descriptor.m_References)
      found |= reference.m_Id == xiiTransparencyResolveDefinitionId();
    if (!found)
      descriptor.m_References.PushBack({xiiTransparencyResolveDefinitionId(), s_TransparencyResource});
  }
  if (opaqueChanged)
  {
    bool found = false;
    for (const auto& reference : descriptor.m_References)
      found |= reference.m_Id == xiiOpaqueCompositionDefinitionId();
    if (!found)
      descriptor.m_References.PushBack({xiiOpaqueCompositionDefinitionId(), s_OpaqueResource});
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
