#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Types/Delegate.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

/// \brief Graphics scaffold for compositing editor/game UI overlays.
class XII_GRAPHICSCORE_DLL xiiRenderGraphUiCompositePass final : public xiiRenderGraphPassBase
{
public:
  using SetupCommandListFunc       = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using ExecuteCommandListFunc     = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using PostExecuteCommandListFunc = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;

  xiiRenderGraphUiCompositePass();

  void SetEnabled(bool bEnabled);

  void SetSceneColorResourceName(xiiHashedString sResourceName);
  void SetUiRenderTargetResourceName(xiiHashedString sResourceName);

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
  xiiHashedString               m_sSceneColorResourceName;
  xiiHashedString               m_sUiRenderTargetResourceName;

  SetupCommandListFunc       m_SetupCommandListFunc;
  ExecuteCommandListFunc     m_ExecuteCommandListFunc;
  PostExecuteCommandListFunc m_PostExecuteCommandListFunc;

  bool m_bEnabled = false;
};
