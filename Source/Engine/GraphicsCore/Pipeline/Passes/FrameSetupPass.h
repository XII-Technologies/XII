#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Types/Delegate.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

/// \brief Graphics-side frame kickoff pass scaffold for constants, markers, and warmup state.
class XII_GRAPHICSCORE_DLL xiiRenderGraphFrameSetupPass final : public xiiRenderGraphPassBase
{
public:
  using SetupCommandListFunc = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using ExecuteCommandListFunc = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;

  xiiRenderGraphFrameSetupPass();

  void SetEnabled(bool bEnabled);
  void SetHasSideEffects(bool bHasSideEffects);
  void SetFrameConstantsResourceName(xiiHashedString sResourceName);

  void SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc);
  void SetExecuteCommandListFunc(ExecuteCommandListFunc executeCommandListFunc);
  void ClearSetupCommandListFunc();
  void ClearExecuteCommandListFunc();

  [[nodiscard]] virtual const xiiRenderGraphPassDescription& GetDescription() const override;
  virtual void                                               RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const override;

private:
  void RebuildResourceLayout();

private:
  xiiRenderGraphPassDescription m_PassDescription;
  xiiHashedString               m_sFrameConstantsResourceName;

  SetupCommandListFunc   m_SetupCommandListFunc;
  ExecuteCommandListFunc m_ExecuteCommandListFunc;

  bool m_bEnabled = false;
};
