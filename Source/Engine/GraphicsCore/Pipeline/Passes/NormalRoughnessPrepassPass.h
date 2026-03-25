#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Types/Delegate.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

/// \brief Optional graphics prepass that writes compact normal-roughness data.
class XII_GRAPHICSCORE_DLL xiiRenderGraphNormalRoughnessPrepassPass final : public xiiRenderGraphPassBase
{
public:
  using SetupCommandListFunc = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using DrawCommandListFunc  = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;

  xiiRenderGraphNormalRoughnessPrepassPass();

  void SetEnabled(bool bEnabled);
  void SetIndirectCommandBufferResourceName(xiiHashedString sResourceName);
  void SetIndirectCountBufferResourceName(xiiHashedString sResourceName);
  void SetDepthResourceName(xiiHashedString sResourceName);
  void SetNormalRoughnessResourceName(xiiHashedString sResourceName);

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
  xiiHashedString               m_sIndirectCommandBufferResourceName;
  xiiHashedString               m_sIndirectCountBufferResourceName;
  xiiHashedString               m_sDepthResourceName;
  xiiHashedString               m_sNormalRoughnessResourceName;

  SetupCommandListFunc m_SetupCommandListFunc;
  DrawCommandListFunc  m_DrawCommandListFunc;

  bool m_bEnabled = false;
};
