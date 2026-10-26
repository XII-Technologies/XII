/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lighting/LightingManager.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/BRDFLutPass.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <GraphicsFoundation/Resources/Texture.h>

struct xiiBRDFLutPass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
xiiBRDFLutPass::xiiBRDFLutPass()  = default;
xiiBRDFLutPass::~xiiBRDFLutPass() = default;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBRDFLutPass, 1, xiiRTTIDefaultAllocator<xiiBRDFLutPass>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Activation", m_Activation)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("RequireActivation", m_bRequireActivation)->AddAttributes(new xiiDefaultValueAttribute(false)),
      XII_MEMBER_PROPERTY("BRDFLut", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    } XII_END_PROPERTIES;
    XII_BEGIN_ATTRIBUTES
    {new xiiCategoryAttribute("Rendering/Lighting")} XII_END_ATTRIBUTES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

void xiiBRDFLutPass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const
{
  const auto first = out_pins.GetCount();
  xiiRenderPipelinePass::GetPins(out_pins);
  for (xiiUInt32 i = first; i < out_pins.GetCount(); ++i)
    if (!out_pins[i].m_bInput && out_pins[i].m_sName == "BRDFLut")
      out_pins[i].m_bOptional = m_bRequireActivation;
}

xiiResult xiiBRDFLutPass::Build(xiiRenderPipelinePassContext& context)
{
  // Activation is setup-time resource presence, not a shader input or GPU dependency.
  if (m_bRequireActivation && !context.GetInputTexture(m_Activation).IsValid())
    return XII_SUCCESS;
  if (xiiGALDevice::GetDefaultDevice()->GetFeatures().m_TextureUAVExtendedFormats != xiiGALDeviceFeatureState::Enabled)
    return context.ReportFailure("BRDF integration requires RG16Float storage-image support (TextureUAVExtendedFormats).");
  if (xiiLightingManager::EnsureBRDFLUTResources().Failed())
    return context.ReportFailure("Could not allocate the device-shared BRDF lookup.");
  const auto texture = xiiLightingManager::GetBRDFLUT();
  const auto ready   = xiiLightingManager::GetBRDFLUTReadyFence();
  if (!texture || !ready)
    return context.ReportFailure("BRDF lookup resources are unavailable.");
  const bool generate = xiiLightingManager::IsBRDFLUTGenerationPending();
  if (generate)
  {
    if (!m_pState)
      m_pState = XII_DEFAULT_NEW(State);
    XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/BRDFLUTGenerate.xiiShader"));
  }
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle m_Output;
    xiiSharedPtr<xiiGALFence>   m_Ready;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Ready = ready;
    if (generate) data.m_Pipeline = m_pState->m_Pipeline;
    data.m_Output = builder.ImportTexture("Split-Sum Lookup", texture, texture->GetResourceState());
    if (generate) data.m_Output = builder.WriteTexture(data.m_Output, xiiGALResourceStateFlags::UnorderedAccess);
    else
    {
      builder.WaitForFence(ready, 1).AssertSuccess();
      // Publish a preserved version establishing external readiness for every
      // consumer queue, including readers whose layout already matches.
      data.m_Output = builder.ReadWriteTexture(data.m_Output, xiiGALResourceStateFlags::ShaderResource);
    } }, [generate](const Data& data, xiiRenderGraphPassContext& execute) {
    // Repeated occurrences built before Execute may all request generation.
    // The graph orders aliases; only the first live occurrence records the work.
    if (!generate || !xiiLightingManager::IsBRDFLUTGenerationPending()) return;
    auto& commands = execute.GetCommandList();
    commands.SetPipelineState(data.m_Pipeline.Borrow());
    auto* output = execute.GetTexture(data.m_Output);
    commands.ResolveAndSetUnorderedAccessTextureView("g_BRDFLutOut", output->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed())
    {
      execute.ReportFailure("Could not bind BRDF integration resources.");
      return;
    }
    const auto size = output->GetDescription().m_Size;
    commands.DispatchCompute({(size.width + 7U) / 8U, (size.height + 7U) / 8U, 1});
    commands.EnqueueSignal(data.m_Ready.Borrow(), 1);
    xiiLightingManager::MarkBRDFLUTGenerated(); });
  return context.SetOutput(m_Output, emitted.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Lighting_BRDFLutPass);
