/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiScreenSpaceReflectionsPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScreenSpaceReflectionsPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_InputSceneDepth{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputGBufferNormal{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputGBufferMaterial{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputGBufferAlbedo{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputBRDFLut{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputHiZClosestPyramid{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputHiZPyramid{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputHDRSceneColor{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputSSRRawTerm{xiiGALResourceFormat::RGBA32Float, false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Automatic};
  xiiRenderPipelineTextureOutputPin m_OutputSceneDepth{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineTextureOutputPin m_OutputGBufferNormal{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineTextureOutputPin m_OutputGBufferMaterial{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineTextureOutputPin m_OutputGBufferAlbedo{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineTextureOutputPin m_OutputBRDFLut{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineTextureOutputPin m_OutputHiZClosestPyramid{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineTextureOutputPin m_OutputHiZPyramid{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineTextureOutputPin m_OutputHDRSceneColor{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineBufferInputPin   m_InputPerFrameCamera{false, xiiGALBindFlags::UniformBuffer};
  /// Retained optional authoring port; spatial tracing has no frame-time dependency.
  xiiRenderPipelineBufferInputPin   m_InputPerFrameGlobal{true, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferOutputPin  m_OutputConstants{false, xiiGALBindFlags::UniformBuffer};
  bool                              m_bAllowAbsentDepth = false;
  float                             m_fMaxDistance      = 50.0f;
  float                             m_fSurfaceBias      = 0.01f;
  void                              GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const override;
  xiiUInt32                         m_uiMaxSteps     = 96;
  float                             m_fThickness     = 0.0025f;
  float                             m_fMaxRoughness  = 0.72f;
  float                             m_fStrideZCutoff = 0.25f;
  xiiResult                         Configure(const xiiVariantDictionary& properties) override;
  xiiScreenSpaceReflectionsPass();
  ~xiiScreenSpaceReflectionsPass();
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;

private:
  struct Data;
  struct State;
  void                Setup(Data& data, xiiRenderGraphBuilder& builder);
  static void         Execute(const Data& data, xiiRenderGraphPassContext& context);
  xiiUniquePtr<State> m_pState;
};
