/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Denoising/LocalLightDenoisePass.h>

xiiLocalLightDenoisePass::xiiLocalLightDenoisePass() :
  xiiTemporalSignalDenoisePass(SignalMode::LocalLighting)
{
}

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLocalLightDenoisePass, 1, xiiRTTIDefaultAllocator<xiiLocalLightDenoisePass>)
{
  using RawAccessor = xiiPropertyAccessor<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureInputPin, &xiiTemporalSignalDenoisePass::m_Raw>;
  using OutputAccessor = xiiPropertyAccessor<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureOutputPin, &xiiTemporalSignalDenoisePass::m_Output>;
  using PreviousAccessor = xiiPropertyAccessor<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureOutputPin, &xiiTemporalSignalDenoisePass::m_Previous>;
  using MaterialAccessor = xiiPropertyAccessor<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureInputPin, &xiiTemporalSignalDenoisePass::m_Material>;
  using ObjectIdAccessor = xiiPropertyAccessor<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureInputPin, &xiiTemporalSignalDenoisePass::m_ObjectId>;
  using DomainAccessor = xiiPropertyAccessor<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureInputPin, &xiiTemporalSignalDenoisePass::m_SampleDomain>;

  XII_BEGIN_PROPERTIES
  {
    (new xiiMemberProperty<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureInputPin>(
      "Lighting", &RawAccessor::GetValue, &RawAccessor::SetValue, &RawAccessor::GetPropertyPointer))->AddFlags(xiiPropertyFlags::ReadOnly),
    (new xiiMemberProperty<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureOutputPin>(
      "FilteredLighting", &OutputAccessor::GetValue, &OutputAccessor::SetValue, &OutputAccessor::GetPropertyPointer))->AddFlags(xiiPropertyFlags::ReadOnly),
    (new xiiMemberProperty<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureOutputPin>(
      "PreviousLighting", &PreviousAccessor::GetValue, &PreviousAccessor::SetValue, &PreviousAccessor::GetPropertyPointer))->AddFlags(xiiPropertyFlags::ReadOnly),
    (new xiiMemberProperty<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureInputPin>(
      "GBufferMaterial", &MaterialAccessor::GetValue, &MaterialAccessor::SetValue, &MaterialAccessor::GetPropertyPointer))->AddFlags(xiiPropertyFlags::ReadOnly),
    (new xiiMemberProperty<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureInputPin>(
      "GBufferObjectId", &ObjectIdAccessor::GetValue, &ObjectIdAccessor::SetValue, &ObjectIdAccessor::GetPropertyPointer))->AddFlags(xiiPropertyFlags::ReadOnly),
    (new xiiMemberProperty<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureInputPin>(
      "ReservoirDomain", &DomainAccessor::GetValue, &DomainAccessor::SetValue, &DomainAccessor::GetPropertyPointer))->AddFlags(xiiPropertyFlags::ReadOnly),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Lighting") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Denoising_Implementation_LocalLightDenoisePass);
