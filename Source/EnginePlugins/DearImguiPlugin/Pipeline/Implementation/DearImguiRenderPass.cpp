/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <DearImguiPlugin/DearImguiPluginPCH.h>

#include <DearImguiPlugin/Implementation/DearImguiContextData.h>
#include <DearImguiPlugin/Pipeline/DearImguiRenderPass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Shader/Shader.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/GUI/DearImguiConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiImguiRenderPass, 1, xiiRTTIDefaultAllocator<xiiImguiRenderPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ColorInput", m_InputColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("Color")),
    XII_MEMBER_PROPERTY("UI", m_InputUI)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("ColorOutput", m_OutputColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("Color")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/GUI") } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

struct xiiImguiRenderPass::Data
{
  xiiSharedPtr<const xiiImguiFrame>              m_pFrame;
  xiiRenderGraphTextureHandle                    m_Color;
  xiiDynamicArray<xiiRenderGraphTextureHandle>   m_Textures;
  xiiRenderGraphBufferHandle                     m_Vertices, m_Indices, m_Constants;
  xiiGALGraphicsPipelineStateCreationDescription m_Pipeline;
  xiiSharedPtr<xiiGALSampler>                    m_pSampler;
  xiiRectFloat                                   m_Viewport;
  xiiImguiConstants                              m_Settings;
  bool                                           m_bReady = false;
};

namespace
{
  template <typename T>
  bool Upload(xiiRenderGraphPassContext& context, xiiRenderGraphBufferHandle buffer, xiiArrayPtr<const T> values, xiiGALResourceStateFlags::Enum state)
  {
    auto& commands = context.GetCommandList();
    {
      xiiGALMapHelper<T> mapped(commands, context.GetBuffer(buffer), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      if (!mapped.GetMappedData())
      {
        xiiLog::Error("Could not upload Dear ImGui frame data.");
        return false;
      }
      xiiMemoryUtils::Copy(mapped.GetMappedData(), values.GetPtr(), values.GetCount());
    }
    xiiGALStateTransitionDescription transition;
    transition.m_pResource       = context.GetBuffer(buffer);
    transition.m_NewState        = state;
    transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
    commands.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
    return true;
  }

  xiiRenderGraphBufferHandle GeometryBuffer(xiiRenderGraphBuilder& builder, xiiStringView name, xiiUInt64 bytes, xiiGALBindFlags::Enum binding, xiiGALResourceStateFlags::Enum state)
  {
    xiiGALBufferCreationDescription desc;
    // Geometric capacity growth lets the device-scoped graph cache reuse allocations across UI size changes.
    desc.m_uiSize         = xiiMath::PowerOfTwo_Ceil(xiiMath::Max<xiiUInt64>(bytes, 4096));
    desc.m_BindFlags      = binding;
    desc.m_Usage          = xiiGALResourceUsage::Dynamic;
    desc.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
    return builder.WriteBuffer(name, desc, state);
  }
} // namespace

xiiResult xiiImguiRenderPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto color = context.GetInputTexture(m_InputColor);
  const auto value = context.GetInputData(m_InputUI);
  const auto frame = value ? value.Downcast<const xiiImguiFrameData>()->m_pFrame : xiiSharedPtr<const xiiImguiFrame>();
  if (!frame || frame->GetCommands().IsEmpty())
    return context.SetOutput(m_OutputColor, color);
  const auto* description = context.m_Graph.GetTextureDescription(color);
  if (!description || description->m_Type != xiiGALResourceDimension::Texture2D || description->m_uiArraySizeOrDepth != 1 || description->m_uiSampleCount != 1 ||
      !description->m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget) ||
      !(description->m_Format == xiiGALResourceFormat::RGBA16Float || description->m_Format == xiiGALResourceFormat::RGBA32Float || xiiGALResourceFormat::IsSrgb(description->m_Format)))
    return context.ReportFailure("Dear ImGui requires a single-sample display-linear SDR or sRGB color target.");
  const auto& rect = frame->GetDisplayRect();
  if (!rect.IsValid() || rect.z <= 0 || rect.w <= 0 || frame->GetVertices().IsEmpty() || frame->GetIndices().IsEmpty())
    return context.ReportFailure("Invalid Dear ImGui display rectangle or geometry.");
  const xiiRectFloat viewport = context.m_pView ? context.m_pView->GetViewport() : xiiRectFloat{0, 0, float(description->m_Size.width), float(description->m_Size.height)};
  if (!xiiMath::IsFinite(viewport.x) || !xiiMath::IsFinite(viewport.y) || !xiiMath::IsFinite(viewport.width) || !xiiMath::IsFinite(viewport.height) ||
      viewport.x < 0 || viewport.y < 0 || viewport.width <= 0 || viewport.height <= 0 ||
      double(viewport.x) + viewport.width > description->m_Size.width || double(viewport.y) + viewport.height > description->m_Size.height)
    return context.ReportFailure("Dear ImGui viewport must fit inside its explicit color target.");
  const auto                                    shaderHandle = xiiShaderPermutationUtilities::PreloadSinglePermutation(xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/GUI/DearImguiPrimitives.xiiShader"), {}, false);
  xiiResourceLock<xiiShaderPermutationResource> shader(shaderHandle, xiiResourceAcquireMode::BlockTillLoaded);
  if (!shader.IsValid() || !shader->IsShaderValid())
    return context.ReportFailure("Dear ImGui shader is unavailable.");
  xiiGALGraphicsPipelineStateCreationDescription pipeline;
  pipeline.m_pPipelineResourceSignature            = shader->GetPipelineResourceSignature();
  pipeline.m_pVertexShader                         = shader->GetGALShader(xiiGALShaderType::Vertex);
  pipeline.m_pPixelShader                          = shader->GetGALShader(xiiGALShaderType::Pixel);
  pipeline.m_GraphicsPipeline.m_pBlendState        = shader->GetBlendState();
  pipeline.m_GraphicsPipeline.m_pRasterizerState   = shader->GetRasterizerState();
  pipeline.m_GraphicsPipeline.m_pDepthStencilState = shader->GetDepthStencilState();
  pipeline.m_GraphicsPipeline.m_PrimitiveTopology  = xiiGALPrimitiveTopology::TriangleList;
  xiiGALInputLayoutCreationDescription layout;
  layout.m_LayoutElements.PushBack({xiiGALInputLayoutSemantic::Position, 0, xiiGALResourceFormat::RG32Float, offsetof(ImDrawVert, pos), sizeof(ImDrawVert), xiiGALInputElementFrequency::PerVertex, 1});
  layout.m_LayoutElements.PushBack({xiiGALInputLayoutSemantic::TexCoord0, 0, xiiGALResourceFormat::RG32Float, offsetof(ImDrawVert, uv), sizeof(ImDrawVert), xiiGALInputElementFrequency::PerVertex, 1});
  layout.m_LayoutElements.PushBack({xiiGALInputLayoutSemantic::Color0, 0, xiiGALResourceFormat::RGBA8UNormalized, offsetof(ImDrawVert, col), sizeof(ImDrawVert), xiiGALInputElementFrequency::PerVertex, 1});
  pipeline.m_GraphicsPipeline.m_pInputLayout = pipeline.m_pVertexShader->CreateInputLayout(layout);
  auto sampler                               = xiiGALDevice::GetDefaultDevice()->CreateSampler({});
  if (!pipeline.m_GraphicsPipeline.m_pInputLayout || !sampler)
    return context.ReportFailure("Dear ImGui input layout or sampler creation failed.");
  xiiDynamicArray<xiiSharedPtr<xiiGALTexture>> textures;
  for (const auto& texture : frame->GetTextures())
  {
    auto image = texture->GetTexture();
    if (!image)
      return context.ReportFailure("Dear ImGui texture upload failed.");
    // Sampling the attachment being written would form an invalid feedback loop.
    textures.PushBack(std::move(image));
  }
  xiiRenderPassResourceScope scope(context);
  const auto                 emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_pFrame = frame;
      data.m_Color = builder.ReadWriteTexture(color, xiiGALResourceStateFlags::RenderTarget);
      data.m_Pipeline = pipeline;
      data.m_pSampler = sampler;
      data.m_Viewport = viewport;
      data.m_Settings.ScaleAndTranslation = xiiVec4(2.0f / rect.z, -2.0f / rect.w, -1.0f - rect.x * 2.0f / rect.z, 1.0f + rect.y * 2.0f / rect.w);
      data.m_Vertices = GeometryBuffer(builder, "UI vertices", frame->GetVertices().GetCount() * sizeof(ImDrawVert), xiiGALBindFlags::VertexBuffer, xiiGALResourceStateFlags::VertexBuffer);
      data.m_Indices = GeometryBuffer(builder, "UI indices", frame->GetIndices().GetCount() * sizeof(ImDrawIdx), xiiGALBindFlags::IndexBuffer, xiiGALResourceStateFlags::IndexBuffer);
      data.m_Constants = GeometryBuffer(builder, "UI constants", sizeof(xiiImguiConstants), xiiGALBindFlags::UniformBuffer, xiiGALResourceStateFlags::ConstantBuffer);
      for (xiiUInt32 i = 0; i < textures.GetCount(); ++i)
      {
        xiiStringBuilder name;
        name.SetFormat("UI texture/{0}", i);
        const auto imported = builder.ImportTexture(name, textures[i], textures[i]->GetResourceState());
        data.m_Textures.PushBack(builder.ReadTexture(imported, xiiGALResourceStateFlags::ShaderResource));
      }
      builder.SetPassAllowMerge(false);
      builder.SetPassRenderPassManaged();
      builder.SetPassPreparation([&data](xiiRenderGraphPassContext& execution) {
        data.m_bReady = Upload(execution, data.m_Vertices, data.m_pFrame->GetVertices(), xiiGALResourceStateFlags::VertexBuffer) &&
          Upload(execution, data.m_Indices, data.m_pFrame->GetIndices(), xiiGALResourceStateFlags::IndexBuffer) &&
          Upload(execution, data.m_Constants, xiiArrayPtr<const xiiImguiConstants>(&data.m_Settings, 1), xiiGALResourceStateFlags::ConstantBuffer);
      }); }, Execute);
  return context.SetOutput(m_OutputColor, emitted.first->m_Color);
}

void xiiImguiRenderPass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  if (!data.m_bReady)
    return;
  auto description                                = data.m_Pipeline;
  description.m_GraphicsPipeline.m_pRenderPass    = context.GetRenderPass();
  description.m_GraphicsPipeline.m_uiSubpassIndex = static_cast<xiiUInt8>(context.GetSubpassIndex());
  auto pipeline                                   = xiiGALPipelineCache::GetPipeline(description);
  if (!pipeline)
  {
    xiiLog::Error("Could not create Dear ImGui graphics pipeline.");
    return;
  }
  auto& commands = context.GetCommandList();
  commands.SetPipelineState(pipeline.Borrow());
  commands.SetViewport(data.m_Viewport);
  xiiGALBuffer* vertices = context.GetBuffer(data.m_Vertices);
  commands.SetVertexBuffers(0, xiiMakeArrayPtr(&vertices, 1), {}, xiiGALSetVertexBufferFlags::Reset, xiiGALStateTransitionMode::Verify);
  commands.SetIndexBuffer(context.GetBuffer(data.m_Indices), 0, xiiGALStateTransitionMode::Verify);
  commands.ResolveAndSetConstantBuffer("xiiImguiConstants", context.GetBuffer(data.m_Constants), xiiGALShaderType::Vertex);
  commands.ResolveAndSetSampler("UITextureSampler", data.m_pSampler.Borrow());
  const auto& display = data.m_pFrame->GetDisplayRect();
  const float sx = data.m_Viewport.width / display.z, sy = data.m_Viewport.height / display.w;
  for (const auto& draw : data.m_pFrame->GetCommands())
  {
    const auto& clip   = draw.m_vClipRect;
    const float left   = xiiMath::Clamp((clip.x - display.x) * sx + data.m_Viewport.x, data.m_Viewport.x, data.m_Viewport.x + data.m_Viewport.width);
    const float top    = xiiMath::Clamp((clip.y - display.y) * sy + data.m_Viewport.y, data.m_Viewport.y, data.m_Viewport.y + data.m_Viewport.height);
    const float right  = xiiMath::Clamp((clip.z - display.x) * sx + data.m_Viewport.x, data.m_Viewport.x, data.m_Viewport.x + data.m_Viewport.width);
    const float bottom = xiiMath::Clamp((clip.w - display.y) * sy + data.m_Viewport.y, data.m_Viewport.y, data.m_Viewport.y + data.m_Viewport.height);
    if (right <= left || bottom <= top)
      continue;
    const auto x = static_cast<xiiUInt32>(xiiMath::Floor(left)), y = static_cast<xiiUInt32>(xiiMath::Floor(top));
    const auto r = static_cast<xiiUInt32>(xiiMath::Ceil(right)), b = static_cast<xiiUInt32>(xiiMath::Ceil(bottom));
    commands.SetScissorRect({x, y, r - x, b - y});
    commands.ResolveAndSetShaderResourceTextureView("UITexture", context.GetTexture(data.m_Textures[draw.m_uiTexture])->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).Failed())
    {
      xiiLog::Error("Could not bind Dear ImGui draw resources.");
      return;
    }
    commands.DrawIndexed({draw.m_uiIndexCount, sizeof(ImDrawIdx) == 2 ? xiiGALValueType::UInt16 : xiiGALValueType::UInt32, 1, draw.m_uiFirstIndex, draw.m_uiBaseVertex});
  }
}

XII_STATICLINK_FILE(DearImguiPlugin, DearImguiPlugin_Pipeline_Implementation_DearImguiRenderPass);
