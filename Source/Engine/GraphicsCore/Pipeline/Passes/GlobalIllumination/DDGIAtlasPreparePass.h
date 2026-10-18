/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIProbeFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
/// Creates private current atlases, copying explicit history or clearing newly invalidated storage.
class XII_GRAPHICSCORE_DLL xiiDDGIAtlasPreparePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDDGIAtlasPreparePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataInputPin     m_Schedule{xiiGetStaticRTTI<xiiDDGIProbeFrameData>()};
  xiiRenderPipelineTextureInputPin  m_PreviousIrradiance{xiiGALResourceFormat::RGBA16Float, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_PreviousDistance{xiiGALResourceFormat::RG32Float, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_Irradiance{xiiGALResourceFormat::RGBA16Float, false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_Distance{xiiGALResourceFormat::RG32Float, false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override;
};
