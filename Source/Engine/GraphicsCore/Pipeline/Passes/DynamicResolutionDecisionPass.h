#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Types/Delegate.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

/// \brief Orchestration scaffold for deciding dynamic resolution scale after frame kickoff.
class XII_GRAPHICSCORE_DLL xiiRenderGraphDynamicResolutionDecisionPass final : public xiiRenderGraphPassBase
{
public:
  using SetupCommandListFunc       = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using ExecuteCommandListFunc     = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using PostExecuteCommandListFunc = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;

  xiiRenderGraphDynamicResolutionDecisionPass();

  void SetEnabled(bool bEnabled);
  void SetHasSideEffects(bool bHasSideEffects);

  void SetFrameTimingResourceName(xiiHashedString sResourceName);
  void SetDynamicResolutionDecisionResourceName(xiiHashedString sResourceName);

  void SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc);
  void SetExecuteCommandListFunc(ExecuteCommandListFunc executeCommandListFunc);
  void SetPostExecuteCommandListFunc(PostExecuteCommandListFunc postExecuteCommandListFunc);
  void ClearSetupCommandListFunc();
  void ClearExecuteCommandListFunc();
  void ClearPostExecuteCommandListFunc();

  [[nodiscard]] virtual const xiiRenderGraphPassDescription& GetDescription() const override;
  virtual void                                               RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const override;

private:
  void RebuildResourceLayout();

private:
  xiiRenderGraphPassDescription m_PassDescription;
  xiiHashedString               m_sFrameTimingResourceName;
  xiiHashedString               m_sDynamicResolutionDecisionResourceName;

  SetupCommandListFunc       m_SetupCommandListFunc;
  ExecuteCommandListFunc     m_ExecuteCommandListFunc;
  PostExecuteCommandListFunc m_PostExecuteCommandListFunc;

  bool m_bEnabled = false;
};
