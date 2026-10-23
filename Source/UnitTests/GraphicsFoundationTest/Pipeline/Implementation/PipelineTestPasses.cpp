/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Foundation/Strings/StringBuilder.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundationTest/Pipeline/PipelineTestPasses.h>

xiiResult xiiIndirectTestExport::Build(xiiRenderPipelinePassContext& context)
{
  const auto pass = context.m_Graph.AddPass<xiiRenderGraphTextureHandle>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) {data=builder.ReadTexture(context.GetInputTexture(m_Input),xiiGALResourceStateFlags::ShaderResource);builder.ExportTexture(data,xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
  return context.SetOutput(m_Output, *pass.first);
}


// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiIndirectTestExport,1,xiiRTTIDefaultAllocator<xiiIndirectTestExport>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Input",m_Input)->AddFlags(xiiPropertyFlags::ReadOnly), XII_MEMBER_PROPERTY("Output",m_Output)->AddFlags(xiiPropertyFlags::ReadOnly), } XII_END_PROPERTIES; } XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiOpaqueTestExport::Build(xiiRenderPipelinePassContext& context)
{
  const auto input = context.GetInputTexture(m_Input);
  if (!input.IsValid())
    return XII_SUCCESS;
  const auto emitted = context.m_Graph.AddPass<xiiRenderGraphTextureHandle>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](xiiRenderGraphTextureHandle& data, xiiRenderGraphBuilder& builder) {
      data = builder.ReadTexture(input, xiiGALResourceStateFlags::CopySource);
      builder.ExportTexture(data, xiiGALResourceStateFlags::CopySource); }, [](const xiiRenderGraphTextureHandle&, xiiRenderGraphPassContext&) {}, true);
  return context.SetOutput(m_Output, *emitted.first);
}


// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiOpaqueTestExport, 1, xiiRTTIDefaultAllocator<xiiOpaqueTestExport>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Input", m_Input)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Output", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiScreenEffectTestSource::Build(xiiRenderPipelinePassContext& context)
{
  struct Data
  {
    xiiRenderGraphTextureHandle m_Color, m_Attributes, m_Depth;
    xiiColor                    m_AttributeColor;
    float                       m_fDepth = 0;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_AttributeColor = m_Attributes;
      data.m_fDepth = m_fDepth;
      xiiGALTextureCreationDescription description;
      description.m_Type = xiiGALResourceDimension::Texture2D;
      description.m_Size = {8, 8};
      description.m_Format = xiiGALResourceFormat::RGBA16Float;
      description.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
      data.m_Color = builder.WriteTexture(xiiStringBuilder(context.m_sPassName, "/Color"), description, xiiGALResourceStateFlags::CopyDestination);
      data.m_Attributes = builder.WriteTexture(xiiStringBuilder(context.m_sPassName, "/Attributes"), description, xiiGALResourceStateFlags::CopyDestination);
      description.m_Format = xiiGALResourceFormat::R32Float;
      data.m_Depth = builder.WriteTexture(xiiStringBuilder(context.m_sPassName, "/Depth"), description, xiiGALResourceStateFlags::CopyDestination); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      execute.GetCommandList().ClearRenderTargetView(execute.GetTexture(data.m_Color)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(4, 4, 4, 1));
      execute.GetCommandList().ClearRenderTargetView(execute.GetTexture(data.m_Attributes)->GetDefaultView(xiiGALTextureViewType::RenderTarget), data.m_AttributeColor);
      execute.GetCommandList().ClearRenderTargetView(execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(data.m_fDepth, 0, 0, 1)); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_ColorOutput, emitted.first->m_Color));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_AttributesOutput, emitted.first->m_Attributes));
  return context.SetOutput(m_DepthOutput, emitted.first->m_Depth);
}


// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScreenEffectTestSource, 1, xiiRTTIDefaultAllocator<xiiScreenEffectTestSource>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("ColorOutput", m_ColorOutput)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinNameAttribute("Color")),
      XII_MEMBER_PROPERTY("AttributesOutput", m_AttributesOutput)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinNameAttribute("Attributes")),
      XII_MEMBER_PROPERTY("DepthOutput", m_DepthOutput)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinNameAttribute("Depth")),
      XII_MEMBER_PROPERTY("Depth", m_fDepth),
      XII_MEMBER_PROPERTY("Attributes", m_Attributes),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiExtractionDemandPass::Build(xiiRenderPipelinePassContext&)
{
  return XII_SUCCESS;
}


// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiExtractionDemandPass, 1, xiiRTTIDefaultAllocator<xiiExtractionDemandPass>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Categories", m_uiCategories),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
