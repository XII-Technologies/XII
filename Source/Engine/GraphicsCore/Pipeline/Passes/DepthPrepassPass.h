#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Types/Delegate.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

/// \brief Graphics-queue scaffold for depth prepass command recording.
///
/// This pass only declares common depth-prepass inputs/outputs and forwards
/// draw logic to caller-supplied callbacks.
class XII_GRAPHICSCORE_DLL xiiRenderGraphDepthPrepassPass final : public xiiRenderGraphPassBase
{
public:
  using SetupCommandListFunc = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using DrawCommandListFunc  = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using PostCommandListFunc  = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;

  xiiRenderGraphDepthPrepassPass();

  void SetEnabled(bool bEnabled);
  void SetHasSideEffects(bool bHasSideEffects);

  void SetVisibleInstancesResourceName(xiiHashedString sResourceName);
  void SetDepthBufferResourceName(xiiHashedString sResourceName);

  void SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc);
  void SetDrawCommandListFunc(DrawCommandListFunc drawCommandListFunc);
  void SetPostCommandListFunc(PostCommandListFunc postCommandListFunc);
  void ClearSetupCommandListFunc();
  void ClearDrawCommandListFunc();
  void ClearPostCommandListFunc();

  [[nodiscard]] XII_ALWAYS_INLINE bool IsEnabled() const { return m_bEnabled; }

  [[nodiscard]] virtual const xiiRenderGraphPassDescription& GetDescription() const override;
  virtual void                                               RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const override;

private:
  void RebuildResourceLayout();

private:
  xiiRenderGraphPassDescription m_PassDescription;

  xiiHashedString m_sVisibleInstancesResourceName;
  xiiHashedString m_sDepthBufferResourceName;

  SetupCommandListFunc m_SetupCommandListFunc;
  DrawCommandListFunc  m_DrawCommandListFunc;
  PostCommandListFunc  m_PostCommandListFunc;

  bool m_bEnabled = false;
};
