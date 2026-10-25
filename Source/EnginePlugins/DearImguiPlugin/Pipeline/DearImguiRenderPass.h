/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <DearImguiPlugin/DearImguiPluginDLL.h>
#include <DearImguiPlugin/DearImguiRenderer.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// SDR overlay in linear light. Connect after color grading/upscaling and before Present.
/// Color must contain display-linear SDR, or use an sRGB render target which GAL decodes for blending.
/// HDR/PQ/HLG and display-encoded non-sRGB targets require a separate composition path and are not supported.
class XII_DEARIMGUIPLUGIN_DLL xiiImguiRenderPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiImguiRenderPass, xiiRenderPipelinePass);

public:
  xiiImguiRenderPass();
  ~xiiImguiRenderPass() override;
  xiiRenderPipelineTextureInputPin  m_InputColor{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::RenderTarget, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineDataInputPin     m_InputUI{xiiGetStaticRTTI<xiiImguiFrameData>(), true};
  xiiRenderPipelineTextureOutputPin m_OutputColor{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::RenderTarget, xiiGALResourceDimension::Texture2D, 1};
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override;

private:
  struct Data;
  struct State;
  xiiUniquePtr<State> m_pState;
  static void         Execute(const Data& data, xiiRenderGraphPassContext& context);
};
