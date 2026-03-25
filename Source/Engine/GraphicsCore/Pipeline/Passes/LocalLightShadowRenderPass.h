#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Types/Delegate.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

/// \brief Graphics scaffold for rendering local-light shadow maps into a packed atlas.
class XII_GRAPHICSCORE_DLL xiiRenderGraphLocalLightShadowRenderPass final : public xiiRenderGraphPassBase
{
public:
  using SetupCommandListFunc       = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using ExecuteCommandListFunc     = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using PostExecuteCommandListFunc = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;

  xiiRenderGraphLocalLightShadowRenderPass();

  void SetEnabled(bool bEnabled);

  void SetLocalShadowDataResourceName(xiiHashedString sResourceName);
  void SetLocalShadowDepthAtlasResourceName(xiiHashedString sResourceName);

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
  xiiHashedString               m_sLocalShadowDataResourceName;
  xiiHashedString               m_sLocalShadowDepthAtlasResourceName;

  SetupCommandListFunc       m_SetupCommandListFunc;
  ExecuteCommandListFunc     m_ExecuteCommandListFunc;
  PostExecuteCommandListFunc m_PostExecuteCommandListFunc;

  bool m_bEnabled = false;
};
