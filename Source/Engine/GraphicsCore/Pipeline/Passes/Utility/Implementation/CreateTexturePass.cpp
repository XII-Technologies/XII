/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Utility/CreateTexturePass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCreateTexturePass, 2, xiiRTTIDefaultAllocator<xiiCreateTexturePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Texture", m_OutputTexture),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Utility")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  class xiiCreateTexturePassPatch_1_2 : public xiiGraphPatch
  {
  public:
    xiiCreateTexturePassPatch_1_2() : xiiGraphPatch("xiiCreateTexturePass", 2) {}

    void Patch(xiiGraphPatchContext&, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
    {
      // Retain authored sizes and formats when opening a document saved before creation moved onto the output pin.
      auto textureId = pNode->GetGuid();
      textureId.HashCombine(xiiUuid::MakeStableUuidFromString("Texture"));
      if (const auto* property = pNode->FindProperty("Texture"); property && property->m_Value.IsA<xiiUuid>())
        textureId = property->m_Value.Get<xiiUuid>();
      auto* texture = pGraph->GetNode(textureId);
      if (!texture)
      {
        texture = pGraph->AddNode(textureId, "xiiRenderPipelineTextureOutputPin", 1);
        pNode->RemoveProperty("Texture");
        pNode->AddProperty("Texture", textureId);
      }
      auto requirementsId = textureId;
      requirementsId.HashCombine(xiiUuid::MakeStableUuidFromString("Requirements"));
      if (const auto* property = texture->FindProperty("Requirements"); property && property->m_Value.IsA<xiiUuid>())
        requirementsId = property->m_Value.Get<xiiUuid>();
      auto* requirements = pGraph->GetNode(requirementsId);
      if (!requirements)
      {
        requirements = pGraph->AddNode(requirementsId, "xiiRenderPipelinePinConstraints", 1);
        texture->RemoveProperty("Requirements");
        texture->AddProperty("Requirements", requirementsId);
      }
      for (const char* name : {"Width", "Height", "Format"})
        if (const auto* property = pNode->FindProperty(name))
        {
          auto* target = xiiStringView(name) == "Format" ? texture : requirements;
          target->RemoveProperty(name);
          target->AddProperty(name, property->m_Value);
          pNode->RemoveProperty(name);
        }
    }
  };

  xiiCreateTexturePassPatch_1_2 s_CreateTexturePassPatch;
} // namespace

xiiResult xiiCreateTexturePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));

  const auto  format       = m_OutputTexture.GetFormat();
  const auto& requirements = m_OutputTexture.GetRequirements();
  if (requirements.m_uiWidth > 65536 || requirements.m_uiHeight > 65536 ||
      (requirements.m_Dimension != xiiGALResourceDimension::Undefined && requirements.m_Dimension != xiiGALResourceDimension::Texture2D) || requirements.m_uiSamples > 1 ||
      format.GetValue() >= xiiGALResourceFormat::ENUM_COUNT ||
      xiiGALResourceFormat::IsDepthFormat(format) || xiiGALResourceFormat::IsTypeless(format) || xiiGALResourceFormat::IsMultiplanar(format) ||
      xiiGALTextureUtilities::GetResourceFormatProperties(format).IsCompressed())
    return XII_FAILURE;

  return XII_SUCCESS;
}

xiiResult xiiCreateTexturePass::Build(xiiRenderPipelinePassContext& context)
{
  xiiGALTextureCreationDescription description;
  const auto                       uiOutput = m_OutputTexture.GetBindingIndex();
  if (uiOutput >= context.m_Pins.GetCount())
    return context.ReportFailure("Texture creation has no configured output pin.");
  const auto& output       = context.m_Pins[uiOutput];
  const auto& requirements = output.m_Constraints;
  description.m_Type       = requirements.m_Dimension;
  if (description.m_Type == xiiGALResourceDimension::Undefined)
    description.m_Type = xiiGALResourceDimension::Texture2D;
  description.m_Size.width  = requirements.m_uiWidth;
  description.m_Size.height = requirements.m_uiHeight;
  if (context.m_pView)
  {
    if (!description.m_Size.width)
      description.m_Size.width = context.m_pView->GetRenderResolutionWidth();
    if (!description.m_Size.height)
      description.m_Size.height = context.m_pView->GetRenderResolutionHeight();
  }
  description.m_Format        = output.m_Format == xiiGALResourceFormat::Unknown ? xiiGALResourceFormat::RGBA16Float : output.m_Format;
  description.m_BindFlags     = requirements.m_RequiredBindFlags | xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  description.m_uiSampleCount = requirements.m_uiSamples ? requirements.m_uiSamples : 1;
  if (!description.m_Size.width || !description.m_Size.height || description.m_Size.width > 65536 || description.m_Size.height > 65536)
    return context.ReportFailure("Texture creation requires a nonempty size of at most 65536 in each dimension.");

  xiiRenderPassResourceScope scope(context);
  // Declaration has no GPU work. The graph culls this empty pass and allocates the texture at its first writer.
  const auto emitted = context.m_Graph.AddPass<xiiRenderGraphTextureHandle>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](xiiRenderGraphTextureHandle& hTexture, xiiRenderGraphBuilder& builder) { hTexture = builder.DeclareTexture("Texture", description); }, [](const xiiRenderGraphTextureHandle&, xiiRenderGraphPassContext&) {});

  return context.SetOutput(m_OutputTexture, *emitted.first);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Utility_Implementation_CreateTexturePass);
