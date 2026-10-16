/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Reusable built-in rendering operations. Composite stages preserve device-dependent rendering
/// choices; individual post-processing passes can be inserted and rewired independently.
class XII_GRAPHICSCORE_DLL xiiBuiltinRenderPipelinePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBuiltinRenderPipelinePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureOutputPin               m_OutputRTFinalGI{xiiGALResourceFormat::RGBA16Float, true};
  xiiRenderPipelineTextureOutputPin               m_OutputRTFinalReflections{xiiGALResourceFormat::RGBA16Float, true};
  xiiRenderPipelineTextureOutputPin               m_OutputVolumetricScattering{xiiGALResourceFormat::RGBA16Float, true};
  xiiRenderPipelineAccelerationStructureOutputPin m_OutputRayScene{true};
  xiiRenderPipelineBufferOutputPin                m_OutputRayMaterials{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin                m_OutputRayGeometry{true, xiiGALBindFlags::ShaderResource};
  xiiString                                       m_sStage = "PrepareFrame";
  xiiBitflags<xiiRenderExtractionCategory>        GetExtractionRequirements() const override;
  xiiBitflags<xiiRenderPipelineFeature>           GetRuntimeFeatures() const override;
  void                                            GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override;
  xiiResult                                       Build(xiiRenderPipelinePassContext& context) override;
  xiiResult                                       ValidatePipeline(const xiiRenderPipelineResourceDescriptor& descriptor, xiiArrayPtr<const xiiUInt32> order, xiiUInt32 index, xiiStringBuilder* error) const override;
};
