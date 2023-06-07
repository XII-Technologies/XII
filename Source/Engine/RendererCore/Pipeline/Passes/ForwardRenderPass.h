#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>

struct xiiForwardRenderShadingQuality
{
  using StorageType = xiiInt8;

  enum Enum
  {
    Normal,
    Simplified,

    Default = Normal,
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiForwardRenderShadingQuality);

/// \brief A standard forward render pass that renders into the color target.
class XII_RENDERERCORE_DLL xiiForwardRenderPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiForwardRenderPass, xiiRenderPipelinePass);

public:
  xiiForwardRenderPass(const char* szName = "ForwardRenderPass");
  ~xiiForwardRenderPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

protected:
  virtual void SetupResources(xiiGALPass* pGALPass, const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs);
  virtual void SetupPermutationVars(const xiiRenderViewContext& renderViewContext);
  virtual void SetupLighting(const xiiRenderViewContext& renderViewContext);

  virtual void RenderObjects(const xiiRenderViewContext& renderViewContext) = 0;

  xiiRenderPipelineNodePassThrougPin m_PinColor;
  xiiRenderPipelineNodePassThrougPin m_PinDepthStencil;

  xiiEnum<xiiForwardRenderShadingQuality> m_ShadingQuality;
};
