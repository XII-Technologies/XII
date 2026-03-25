#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Types/Delegate.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

/// \brief Graphics scaffold for depth pre-pass of transparent geometry (sorted buckets or peel layer 0).
class XII_GRAPHICSCORE_DLL xiiRenderGraphTransparencyDepthPass final : public xiiRenderGraphPassBase
{
public:
  using SetupCommandListFunc       = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using ExecuteCommandListFunc     = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using PostExecuteCommandListFunc = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;

  xiiRenderGraphTransparencyDepthPass();

  void SetEnabled(bool bEnabled);

  void SetTransparentDrawListResourceName(xiiHashedString sResourceName);
  void SetTransparencyDepthResourceName(xiiHashedString sResourceName);

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
  xiiHashedString               m_sTransparentDrawListResourceName;
  xiiHashedString               m_sTransparencyDepthResourceName;

  SetupCommandListFunc       m_SetupCommandListFunc;
  ExecuteCommandListFunc     m_ExecuteCommandListFunc;
  PostExecuteCommandListFunc m_PostExecuteCommandListFunc;

  bool m_bEnabled = false;
};
