/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
/// Clears a scissored region of a supplied depth image, preserving all other atlas texels.
class XII_GRAPHICSCORE_DLL xiiDepthRegionClearPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDepthRegionClearPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_Input{xiiGALResourceFormat::D32Float, false, xiiGALBindFlags::DepthStencil, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_Output{xiiGALResourceFormat::D32Float, false, xiiGALBindFlags::DepthStencil, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::ReadWrite};
  xiiVec4U32                        m_Viewport = xiiVec4U32::MakeZero();
  float                             m_fDepth   = 0;
  xiiResult                         Configure(const xiiVariantDictionary& properties) override;
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override;

private:
  xiiShaderPermutationResourceHandle m_Shader;
  xiiSharedPtr<xiiGALBuffer>         m_Constants;
  float                              m_fCachedDepth = 0;
};
