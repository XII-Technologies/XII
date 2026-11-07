/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/AmbientOcclusion/AmbientOcclusionTemporalPass.h>

xiiAmbientOcclusionTemporalPass::xiiAmbientOcclusionTemporalPass() : xiiTemporalSignalDenoisePass(SignalMode::Occlusion) {}
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAmbientOcclusionTemporalPass,1,xiiRTTIDefaultAllocator<xiiAmbientOcclusionTemporalPass>)
{
  using RawAccessor = xiiPropertyAccessor<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureInputPin, &xiiTemporalSignalDenoisePass::m_Raw>;
  using OutputAccessor = xiiPropertyAccessor<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureOutputPin, &xiiTemporalSignalDenoisePass::m_Output>;
  using PreviousAccessor = xiiPropertyAccessor<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureOutputPin, &xiiTemporalSignalDenoisePass::m_Previous>;

  XII_BEGIN_PROPERTIES
  {
    (new xiiMemberProperty<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureInputPin>(
      "RawAO", &RawAccessor::GetValue, &RawAccessor::SetValue, &RawAccessor::GetPropertyPointer))->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    (new xiiMemberProperty<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureOutputPin>(
      "StableAO", &OutputAccessor::GetValue, &OutputAccessor::SetValue, &OutputAccessor::GetPropertyPointer))->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    (new xiiMemberProperty<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureOutputPin>(
      "PreviousAO", &PreviousAccessor::GetValue, &PreviousAccessor::SetValue, &PreviousAccessor::GetPropertyPointer))->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/AmbientOcclusion") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_AmbientOcclusion_AmbientOcclusionTemporalPass);
