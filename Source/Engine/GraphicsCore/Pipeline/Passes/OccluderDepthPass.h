#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Types/Delegate.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

/// \brief Graphics scaffold for low-cost occluder-only depth rendering.
class XII_GRAPHICSCORE_DLL xiiRenderGraphOccluderDepthPass final : public xiiRenderGraphPassBase
{
public:
  using SetupCommandListFunc = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using DrawCommandListFunc  = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;

  xiiRenderGraphOccluderDepthPass();

  void SetEnabled(bool bEnabled);
  void SetOccluderInstancesResourceName(xiiHashedString sResourceName);
  void SetOccluderDepthResourceName(xiiHashedString sResourceName);

  void SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc);
  void SetDrawCommandListFunc(DrawCommandListFunc drawCommandListFunc);
  void ClearSetupCommandListFunc();
  void ClearDrawCommandListFunc();

  [[nodiscard]] virtual const xiiRenderGraphPassDescription& GetDescription() const override;
  virtual void                                               RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const override;

private:
  void RebuildResourceLayout();

private:
  xiiRenderGraphPassDescription m_PassDescription;
  xiiHashedString               m_sOccluderInstancesResourceName;
  xiiHashedString               m_sOccluderDepthResourceName;

  SetupCommandListFunc m_SetupCommandListFunc;
  DrawCommandListFunc  m_DrawCommandListFunc;

  bool m_bEnabled = false;
};
