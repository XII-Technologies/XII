/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/DepthPyramidPass.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Visibility/DepthPyramidConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDepthPyramidPass, 1, xiiRTTIDefaultAllocator<xiiDepthPyramidPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SceneDepth", m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("DepthPyramid", m_Pyramid)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ClosestDepth", m_bClosestDepth)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("AllowAbsentDepth", m_bAllowAbsentDepth)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("AsyncCompute", m_bAsyncCompute)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Visibility") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
void xiiDepthPyramidPass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const
{
  const auto first = out_pins.GetCount();
  xiiRenderPipelinePass::GetPins(out_pins);
  for (xiiUInt32 i = first; i < out_pins.GetCount(); ++i)
    if (out_pins[i].m_sName == "SceneDepth" || out_pins[i].m_sName == "DepthPyramid")
      out_pins[i].m_bOptional = m_bAllowAbsentDepth;
}

xiiResult xiiDepthPyramidPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto  input  = context.GetInputTexture(m_Depth);
  if (!input.IsValid() && m_bAllowAbsentDepth)
    return XII_SUCCESS;
  const auto* source = context.m_Graph.GetTextureDescription(input);
  if (!source || (source->m_Format != xiiGALResourceFormat::D32Float && source->m_Format != xiiGALResourceFormat::R32Float) || source->m_Type != xiiGALResourceDimension::Texture2D || source->m_uiArraySizeOrDepth != 1 || source->m_uiSampleCount != 1 || !source->m_Size.width || !source->m_Size.height || !source->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
    return context.ReportFailure("Depth pyramid requires a nonempty single-sample D32 or R32Float 2D depth image.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Visibility/DepthPyramid.xiiShader"));
  const auto& limits = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties;
  if ((source->m_Size.width + 7ULL) / 8 > limits.m_uiMaxThreadGroupCountX || (source->m_Size.height + 7ULL) / 8 > limits.m_uiMaxThreadGroupCountY)
    return context.ReportFailure("Depth pyramid exceeds compute dispatch dimensions.");
  auto target             = *source;
  target.m_Format         = xiiGALResourceFormat::R32Float;
  target.m_BindFlags      = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  target.m_Usage          = xiiGALResourceUsage::Default;
  target.m_CPUAccessFlags = xiiGALCPUAccessFlag::None;
  target.m_MiscFlags      = {};
  target.m_uiMipLevels    = 1;
  for (xiiUInt32 extent = xiiMath::Max(target.m_Size.width, target.m_Size.height); extent > 1; extent >>= 1)
    ++target.m_uiMipLevels;
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle              m_Source, m_Output;
    xiiRenderGraphBufferHandle               m_Constants;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
    xiiGALTextureCreationDescription         m_Description;
    bool                                     m_bClosest;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Source = builder.ReadTexture(input, xiiGALResourceStateFlags::ShaderResource);
      data.m_Output = builder.WriteTexture("Reversed Depth Pyramid", target, xiiGALResourceStateFlags::UnorderedAccess);
      data.m_Constants = xiiRenderPassUtils::Constants(builder, "Depth Reduction Constants", sizeof(xiiDepthPyramidConstants));
      data.m_Description = target; data.m_Pipeline = m_Pipeline; data.m_bClosest = m_bClosestDepth; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      auto* output = execute.GetTexture(data.m_Output);
      // Temporary per-mip layouts are owned locally; restore the graph's whole-image state.
      const auto transitionMip = [&](xiiUInt32 mip, xiiBitflags<xiiGALResourceStateFlags> before, xiiBitflags<xiiGALResourceStateFlags> after) {
        xiiGALStateTransitionDescription transition;
        transition.m_pResource = output;
        transition.m_uiFirstMipLevel = mip; transition.m_uiMipLevelCount = 1;
        transition.m_OldState = before; transition.m_NewState = after;
        commands.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
      };
      xiiDynamicArray<xiiSharedPtr<xiiGALTextureView>> reads, writes;
      reads.SetCount(data.m_Description.m_uiMipLevels); writes.SetCount(data.m_Description.m_uiMipLevels);
      for (xiiUInt32 mip = 0; mip < reads.GetCount(); ++mip)
      {
        xiiGALTextureViewCreationDescription view;
        view.m_ViewType = xiiGALTextureViewType::ShaderResource;
        view.m_uiMostDetailedMip = mip; view.m_uiMipLevelCount = 1;
        reads[mip] = output->CreateView(view);
        view.m_ViewType = xiiGALTextureViewType::UnorderedAccess;
        writes[mip] = output->CreateView(view);
        if (!reads[mip] || !writes[mip]) { execute.ReportFailure("Could not create depth-pyramid mip views."); return; }
      }
      output->SetResourceState(xiiGALResourceStateFlags::Unknown);
      commands.SetPipelineState(data.m_Pipeline.Borrow());
      commands.ResolveAndSetConstantBuffer("xiiDepthPyramidConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
      auto size = data.m_Description.m_Size;
      for (xiiUInt32 mip = 0; mip < reads.GetCount(); ++mip)
      {
        const xiiSizeU32 next = mip ? xiiSizeU32{xiiMath::Max(size.width >> 1U, 1U), xiiMath::Max(size.height >> 1U, 1U)} : size;
        {
          xiiGALMapHelper<xiiDepthPyramidConstants> mapped(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
          mapped->SrcSize = {size.width, size.height}; mapped->DstSize = {next.width, next.height}; mapped->Reduce = mip ? (data.m_bClosest ? 2U : 1U) : 0U;
        }
        xiiGALStateTransitionDescription uniform;
        uniform.m_pResource = execute.GetBuffer(data.m_Constants);
        uniform.m_NewState = xiiGALResourceStateFlags::ConstantBuffer; uniform.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
        commands.TransitionResourceStates(xiiMakeArrayPtr(&uniform, 1));
        const auto sourceView = mip ? reads[mip - 1] : execute.GetTexture(data.m_Source)->GetDefaultView(xiiGALTextureViewType::ShaderResource);
        commands.ResolveAndSetShaderResourceTextureView("g_HiZSource", sourceView.Borrow(), xiiGALShaderType::Compute);
        commands.ResolveAndSetUnorderedAccessTextureView("g_HiZDestination", writes[mip].Borrow(), xiiGALShaderType::Compute);
        if (commands.CommitShaderResources(xiiGALStateTransitionMode::None).Failed())
        {
          for (xiiUInt32 previous = 0; previous < mip; ++previous)
            transitionMip(previous, xiiGALResourceStateFlags::ShaderResource, xiiGALResourceStateFlags::UnorderedAccess);
          output->SetResourceState(xiiGALResourceStateFlags::UnorderedAccess);
          execute.ReportFailure("Could not bind depth-pyramid reduction resources.");
          return;
        }
        commands.DispatchCompute({(next.width + 7U) / 8U, (next.height + 7U) / 8U, 1});
        if (mip + 1 < reads.GetCount()) transitionMip(mip, xiiGALResourceStateFlags::UnorderedAccess, xiiGALResourceStateFlags::ShaderResource);
        size = next;
      }
      for (xiiUInt32 mip = 0; mip + 1 < reads.GetCount(); ++mip)
        transitionMip(mip, xiiGALResourceStateFlags::ShaderResource, xiiGALResourceStateFlags::UnorderedAccess);
      output->SetResourceState(xiiGALResourceStateFlags::UnorderedAccess); });
  return context.SetOutput(m_Pyramid, emitted.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Visibility_DepthPyramidPass);
