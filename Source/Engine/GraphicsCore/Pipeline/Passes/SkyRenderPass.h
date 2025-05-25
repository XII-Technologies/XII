#pragma once

#include <GraphicsCore/Pipeline/Passes/ForwardRenderPass.h>

/// \brief A forward render pass that renders all sky objects into the color target.
class XII_GRAPHICSCORE_DLL xiiSkyRenderPass : public xiiForwardRenderPass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkyRenderPass, xiiForwardRenderPass);

public:
  xiiSkyRenderPass(xiiStringView sName = "SkyRenderPass");
  ~xiiSkyRenderPass();

protected:
  virtual void RenderObjects(const xiiRenderViewContext& renderViewContext, xiiSharedPtr<xiiGALCommandList> pCommandList) override;
};
