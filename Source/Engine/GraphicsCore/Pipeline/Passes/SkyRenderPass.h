#pragma once

#include <GraphicsCore/Pipeline/Passes/ForwardPass.h>

class XII_GRAPHICSCORE_DLL xiiSkyRenderPass : public xiiForwardRenderPass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkyRenderPass, xiiForwardRenderPass);

public:
  xiiSkyRenderPass(xiiStringView sName = "SkyRenderPass");
  ~xiiSkyRenderPass();

protected:
  virtual void RenderObjects(const xiiRenderViewContext& renderViewContext) override;
};
