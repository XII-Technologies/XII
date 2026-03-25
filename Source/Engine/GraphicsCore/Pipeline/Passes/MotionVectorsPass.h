#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Types/Delegate.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

/// \brief Graphics scaffold for motion vector generation.
class XII_GRAPHICSCORE_DLL xiiRenderGraphMotionVectorsPass final : public xiiRenderGraphPassBase
{
public:
  using SetupCommandListFunc = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using DrawCommandListFunc  = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;

  xiiRenderGraphMotionVectorsPass();

  void SetEnabled(bool bEnabled);
  void SetDepthResourceName(xiiHashedString sResourceName);
  void SetMotionVectorsResourceName(xiiHashedString sResourceName);

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
  xiiHashedString               m_sDepthResourceName;
  xiiHashedString               m_sMotionVectorsResourceName;

  SetupCommandListFunc m_SetupCommandListFunc;
  DrawCommandListFunc  m_DrawCommandListFunc;

  bool m_bEnabled = false;
};
