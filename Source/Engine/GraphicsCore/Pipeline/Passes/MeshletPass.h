#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

/// \brief Pass for executing Meshlet-based rendering, which differs from traditional drawing
/// by using Task/Mesh shaders (DispatchMesh) instead of vertex/index buffers.
class XII_GRAPHICSCORE_DLL xiiMeshletPass : public xiiRenderGraphPass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshletPass, xiiRenderGraphPass);

public:
  xiiMeshletPass();
  ~xiiMeshletPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual void InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

private:
  xiiGALPipelineStateHandle m_hMeshletPSO;
};

/// \brief Extension of Meshlet rendering for shadow cascade generation.
class XII_GRAPHICSCORE_DLL xiiMeshletShadowPass : public xiiRenderGraphPass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshletShadowPass, xiiRenderGraphPass);

public:
  xiiMeshletShadowPass();
  ~xiiMeshletShadowPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual void InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

private:
  xiiGALPipelineStateHandle m_hMeshletShadowPSO;
};
