/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Lighting/DisplayOutput.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiPresentPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPresentPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_InputGradedColor{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputTarget{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::RenderTarget, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferOutputPin  m_OutputFinalBlitConstants{true};
  xiiRenderPipelineTextureOutputPin m_OutputGradedColor{xiiGALResourceFormat::Unknown, true, {}, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputViewOutput{xiiGALResourceFormat::Unknown, true, {}, xiiGALResourceDimension::Texture2D, 1};

  xiiPresentPass();
  ~xiiPresentPass();
  /// A containing pipeline can compose overlays before performing its own presentation.
  bool                          m_bEnabled         = true;
  bool                          m_bUseViewDefaults = false;
  xiiEnum<xiiDisplayOutputMode> m_OutputMode;
  float                         m_fPaperWhiteNits     = 203.0f;
  float                         m_fMaximumDisplayNits = 1000.0f;
  xiiResult                     Configure(const xiiVariantDictionary& properties) override;

  xiiResult Build(xiiRenderPipelinePassContext& context) override;

private:
  struct Data;
  struct State;
  static void         Execute(const Data& data, xiiRenderGraphPassContext& context);
  xiiUniquePtr<State> m_pState;
};
