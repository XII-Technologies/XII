/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/IrradianceDenoisePass.h>

xiiIrradianceDenoisePass::xiiIrradianceDenoisePass() : xiiTemporalSignalDenoisePass(1) {}
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiIrradianceDenoisePass,1,xiiRTTIDefaultAllocator<xiiIrradianceDenoisePass>)
{
  using RawAccessor = xiiPropertyAccessor<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureInputPin, &xiiTemporalSignalDenoisePass::m_Raw>;
  using OutputAccessor = xiiPropertyAccessor<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureOutputPin, &xiiTemporalSignalDenoisePass::m_Output>;
  using PreviousAccessor = xiiPropertyAccessor<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureOutputPin, &xiiTemporalSignalDenoisePass::m_Previous>;
  using MaterialAccessor = xiiPropertyAccessor<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureInputPin, &xiiTemporalSignalDenoisePass::m_Material>;
  using ObjectIdAccessor = xiiPropertyAccessor<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureInputPin, &xiiTemporalSignalDenoisePass::m_ObjectId>;

  XII_BEGIN_PROPERTIES
  {
    (new xiiMemberProperty<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureInputPin>(
      "Irradiance", &RawAccessor::GetValue, &RawAccessor::SetValue, &RawAccessor::GetPropertyPointer))->AddFlags(xiiPropertyFlags::ReadOnly),
    (new xiiMemberProperty<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureOutputPin>(
      "FilteredIrradiance", &OutputAccessor::GetValue, &OutputAccessor::SetValue, &OutputAccessor::GetPropertyPointer))->AddFlags(xiiPropertyFlags::ReadOnly),
    (new xiiMemberProperty<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureOutputPin>(
      "PreviousIrradiance", &PreviousAccessor::GetValue, &PreviousAccessor::SetValue, &PreviousAccessor::GetPropertyPointer))->AddFlags(xiiPropertyFlags::ReadOnly),
    (new xiiMemberProperty<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureInputPin>(
      "GBufferMaterial", &MaterialAccessor::GetValue, &MaterialAccessor::SetValue, &MaterialAccessor::GetPropertyPointer))->AddFlags(xiiPropertyFlags::ReadOnly),
    (new xiiMemberProperty<xiiTemporalSignalDenoisePass, xiiRenderPipelineTextureInputPin>(
      "GBufferObjectId", &ObjectIdAccessor::GetValue, &ObjectIdAccessor::SetValue, &ObjectIdAccessor::GetPropertyPointer))->AddFlags(xiiPropertyFlags::ReadOnly),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/GlobalIllumination") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_GlobalIllumination_Implementation_IrradianceDenoisePass);
