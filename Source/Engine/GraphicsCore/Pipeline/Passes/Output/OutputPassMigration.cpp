/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <GraphicsCore/GraphicsCorePCH.h>

namespace
{
  xiiStringView OperationType(xiiStringView stage)
  {
    if (stage == "Upscale")
      return "xiiUpscalePass";
    if (stage == "Bloom")
      return "xiiBloomPass";
    if (stage == "ToneMapping")
      return "xiiToneMappingPass";
    if (stage == "ColorGrading")
      return "xiiColorGradingPass";
    if (stage == "Present")
      return "xiiPresentPass";
    if (stage == "LuminanceHistogram")
      return "xiiLuminanceHistogramPass";
    if (stage == "AutoExposure")
      return "xiiAutoExposurePass";
    if (stage == "TemporalAntiAliasing")
      return "xiiTemporalAntiAliasingPass";
    return {};
  }

  /// Source document conversion preserves node, pin, connection and layout IDs.
  class OutputOperationSourcePatch : public xiiGraphPatch
  {
  public:
    OutputOperationSourcePatch() : xiiGraphPatch("xiiBuiltinRenderPipelinePass", 3) {}
    void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph*, xiiAbstractObjectNode* node) const override
    {
      const auto* stage = node->FindProperty("Stage");
      if (!stage || !stage->m_Value.IsA<xiiString>())
        return;
      const auto type = OperationType(stage->m_Value.Get<xiiString>());
      if (type.IsEmpty())
        return;
      node->RemoveProperty("Stage");
      if (type != "xiiUpscalePass" && type != "xiiTemporalAntiAliasingPass")
        node->AddProperty("UseViewDefaults", true);
      context.RenameClass(type, 1);
    }
  };
  static OutputOperationSourcePatch s_SourcePatch;

  /// Compiled resource descriptors keep type keys and reflected properties as payload.
  /// Convert obsolete stage payloads before native descriptor deserialization.
  class OutputOperationDescriptorPatch : public xiiGraphPatch
  {
  public:
    OutputOperationDescriptorPatch() : xiiGraphPatch({}, 1, PatchType::GraphPatch) {}
    void Patch(xiiGraphPatchContext&, xiiAbstractObjectGraph* graph, xiiAbstractObjectNode*) const override
    {
      for (auto entry : graph->GetAllNodes())
      {
        auto* node = entry.Value();
        if (node->GetType() != "xiiRenderPipelinePassDescriptor" && node->GetType() != "xiiGraphDefinitionNode")
          continue;
        const auto* type       = node->FindProperty("Type");
        const auto* properties = node->FindProperty("Properties");
        if (!type || type->m_Value != xiiVariant("xiiBuiltinRenderPipelinePass") || !properties || !properties->m_Value.IsA<xiiVariantDictionary>())
          continue;
        auto        dictionary = properties->m_Value.Get<xiiVariantDictionary>();
        const auto* stage      = dictionary.GetValue("Stage");
        if (!stage || !stage->IsA<xiiString>())
          continue;
        const auto operation = OperationType(stage->Get<xiiString>());
        if (operation.IsEmpty())
          continue;
        dictionary.Remove("Stage");
        if (operation != "xiiUpscalePass" && operation != "xiiTemporalAntiAliasingPass")
          dictionary.Insert("UseViewDefaults", true);
        node->ChangeProperty("Type", xiiString(operation));
        node->ChangeProperty("Properties", dictionary);
      }
    }
  };
  static OutputOperationDescriptorPatch s_DescriptorPatch;
} // namespace

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Output_OutputPassMigration);
