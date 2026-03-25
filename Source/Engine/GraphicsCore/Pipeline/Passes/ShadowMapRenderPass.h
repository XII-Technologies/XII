#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Types/Delegate.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

/// \brief Graphics scaffold for rendering directional shadow maps into the shadow atlas.
class XII_GRAPHICSCORE_DLL xiiRenderGraphShadowMapRenderPass final : public xiiRenderGraphPassBase
{
public:
  using SetupCommandListFunc       = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using ExecuteCommandListFunc     = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using PostExecuteCommandListFunc = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;

  xiiRenderGraphShadowMapRenderPass();

  void SetEnabled(bool bEnabled);

  void SetShadowVisibleListResourceName(xiiHashedString sResourceName);
  void SetShadowVisibleCountResourceName(xiiHashedString sResourceName);
  void SetShadowCascadeDataResourceName(xiiHashedString sResourceName);
  void SetShadowDepthAtlasResourceName(xiiHashedString sResourceName);

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
  xiiHashedString               m_sShadowVisibleListResourceName;
  xiiHashedString               m_sShadowVisibleCountResourceName;
  xiiHashedString               m_sShadowCascadeDataResourceName;
  xiiHashedString               m_sShadowDepthAtlasResourceName;

  SetupCommandListFunc       m_SetupCommandListFunc;
  ExecuteCommandListFunc     m_ExecuteCommandListFunc;
  PostExecuteCommandListFunc m_PostExecuteCommandListFunc;

  bool m_bEnabled = false;
};
